// SPDX-License-Identifier: MIT
#include "lucidcairn_ota.h"
#include "ota_policy.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "mbedtls/sha256.h"
#include <cstring>
#include <strings.h>

namespace lucidcairn::ota {
namespace {
// Reuse ESP-IDF's HTTP/TLS, image validator, inactive-slot writer and otadata
// selection. Split end() from set_boot_partition() to check the manifest first.
class Http {
public:
    esp_http_client_handle_t client = nullptr;
    bool identity = true;
    int status = 0;
    int64_t length = -1;
    int64_t started = 0;
    ~Http() { if (client) esp_http_client_cleanup(client); }
    bool Open(const std::string& url, const Config& config, bool authorize) {
        if (HttpsOrigin(url).empty()) return false;
        esp_http_client_config_t options = {};
        options.url = url.c_str();
        options.timeout_ms = 15000;
        options.buffer_size = 4096;
        options.crt_bundle_attach = esp_crt_bundle_attach;
        options.disable_auto_redirect = true;
        options.user_data = this;
        options.event_handler = [](esp_http_client_event_t* event) -> esp_err_t {
            auto* self = static_cast<Http*>(event->user_data);
            if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key &&
                strcasecmp(event->header_key, "Content-Encoding") == 0 &&
                (!event->header_value || strcasecmp(event->header_value, "identity") != 0)) {
                self->identity = false;
            }
            return ESP_OK;
        };
        client = esp_http_client_init(&options);
        if (!client) return false;
        if (esp_http_client_set_header(client, "Accept-Encoding", "identity") != ESP_OK) return false;
        if (authorize) {
            const auto bearer = "Bearer " + config.device_token;
            if (esp_http_client_set_header(client, "Authorization", bearer.c_str()) != ESP_OK ||
                esp_http_client_set_header(client, "X-Device-ID", config.device_id.c_str()) != ESP_OK) return false;
        }
        started = esp_timer_get_time();
        if (esp_http_client_open(client, 0) != ESP_OK) return false;
        length = esp_http_client_fetch_headers(client);
        status = esp_http_client_get_status_code(client);
        return length >= 0 && identity && (status == 200 || status == 204);
    }
    int Read(uint8_t* buffer, size_t capacity) {
        if (esp_timer_get_time() - started > 180LL * 1000 * 1000) return -1;
        const int count = esp_http_client_read(client, reinterpret_cast<char*>(buffer), capacity);
        if (count == 0 && !esp_http_client_is_complete_data_received(client)) return -1;
        return count;
    }
};

bool ExpectedLayout(const esp_partition_t* running, const esp_partition_t* target) {
    if (!running || !target || target->address == running->address) return false;
    const auto* a = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    const auto* b = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    const auto* data = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    const auto* nvs = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs");
    const auto* assets = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "assets");
    return a && b && data && nvs && assets &&
        a->address == 0x20000 && b->address == 0x410000 && a->size == kSlotBytes && b->size == kSlotBytes &&
        data->address == 0xd000 && data->size == 0x2000 && nvs->address == 0x9000 && nvs->size == 0x4000 &&
        assets->address == 0x800000 && assets->size == 0x800000 &&
        (running->address == a->address || running->address == b->address) &&
        (target->address == a->address || target->address == b->address);
}

class EspInstall final : public InstallIo {
    const Config& config_;
    const esp_partition_t* target_;
    Http http_;
    esp_ota_handle_t handle_ = 0;
    bool active_ = false;
    mbedtls_sha256_context sha_;
public:
    EspInstall(const Config& config, const esp_partition_t* target) : config_(config), target_(target) {
        mbedtls_sha256_init(&sha_);
    }
    ~EspInstall() override { Abort(); mbedtls_sha256_free(&sha_); }
    bool Open(const Manifest& manifest) override {
        // Signed object URLs may use another origin. Never send the device token
        // there, and never follow redirects (including same-origin redirects).
        const bool same = HttpsOrigin(manifest.download_url) == HttpsOrigin(config_.manifest_url);
        if (!http_.Open(manifest.download_url, config_, same) || http_.status != 200) return false;
        return esp_http_client_is_chunked_response(http_.client) ||
            http_.length == static_cast<int64_t>(manifest.size);
    }
    bool Begin(size_t size) override {
        if (size > target_->size || mbedtls_sha256_starts(&sha_, 0) != 0) return false;
        active_ = esp_ota_begin(target_, size, &handle_) == ESP_OK;
        return active_;
    }
    int Read(uint8_t* bytes, size_t size) override { return http_.Read(bytes, size); }
    bool Write(const uint8_t* bytes, size_t size) override { return esp_ota_write(handle_, bytes, size) == ESP_OK; }
    bool Hash(const uint8_t* bytes, size_t size) override { return mbedtls_sha256_update(&sha_, bytes, size) == 0; }
    std::string Digest() override {
        uint8_t digest[32];
        if (mbedtls_sha256_finish(&sha_, digest) != 0) return {};
        constexpr char hex[] = "0123456789abcdef";
        std::string text(64, '0');
        for (size_t i = 0; i < sizeof(digest); ++i) {
            text[i * 2] = hex[digest[i] >> 4];
            text[i * 2 + 1] = hex[digest[i] & 15];
        }
        return text;
    }
    bool Finish(const Manifest& manifest) override {
        const esp_err_t result = esp_ota_end(handle_);
        active_ = false;  // IDF frees the handle even if validation fails.
        if (result != ESP_OK) return false;
        esp_app_desc_t desc = {};
        if (esp_ota_get_partition_description(target_, &desc) != ESP_OK) return false;
        const std::string version = manifest.version[0] == 'v' ? manifest.version.substr(1) : manifest.version;
        return strnlen(desc.version, sizeof(desc.version)) < sizeof(desc.version) &&
            strnlen(desc.project_name, sizeof(desc.project_name)) < sizeof(desc.project_name) &&
            version == desc.version && std::strcmp(desc.project_name, "xiaozhi") == 0;
    }
    bool SelectBoot() override { return esp_ota_set_boot_partition(target_) == ESP_OK; }
    void Abort() override {
        if (active_) { esp_ota_abort(handle_); active_ = false; }
    }
};
}

CheckResult CheckAndInstall(const Config& config, Power power) {
    if (HttpsOrigin(config.manifest_url).empty() || !ValidToken(config.device_token) ||
        !ValidToken(config.device_id) || config.device_id.size() > 64) return CheckResult::Disabled;
    if (PendingVerify()) return CheckResult::Rejected;
    const auto* running = esp_ota_get_running_partition();
    const auto* boot = esp_ota_get_boot_partition();
    const auto* target = esp_ota_get_next_update_partition(nullptr);
    // Do not overwrite a staged image, an active slot, or an unknown layout.
    if (!ExpectedLayout(running, target) || !boot || boot->address != running->address) return CheckResult::Rejected;
    Http manifest_http;
    if (!manifest_http.Open(config.manifest_url, config, true)) return CheckResult::Failed;
    if (manifest_http.status == 204) return CheckResult::NoUpdate;
    if (manifest_http.length > static_cast<int64_t>(kMaxManifestBytes)) return CheckResult::Rejected;
    std::string text;
    uint8_t bytes[512];
    for (;;) {
        const int count = manifest_http.Read(bytes, sizeof(bytes));
        if (count < 0) return CheckResult::Failed;
        if (count == 0) break;
        if (text.size() + count > kMaxManifestBytes) return CheckResult::Rejected;
        text.append(reinterpret_cast<char*>(bytes), count);
    }
    Manifest manifest;
    if (!ParseManifest(text, manifest)) return CheckResult::Rejected;
    if (!NewerVersion(manifest.version, esp_app_get_description()->version)) return CheckResult::NoUpdate;
    if (!PowerAllowsUpdate(power.known, power.percent, power.external, manifest.minimum_battery_percent))
        return CheckResult::Rejected;
    EspInstall io(config, target);
    return Install(manifest, io) == InstallResult::Installed ? CheckResult::Installed : CheckResult::Failed;
}
}  // namespace lucidcairn::ota
