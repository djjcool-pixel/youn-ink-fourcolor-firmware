// SPDX-License-Identifier: MIT
#include "feature_registry.h"
#include "lucidcairn_ota.h"
#include "ota_policy.h"
#include "application.h"
#include "board.h"
#include "boards/zectrix-s3-epaper-4.2/config.h"
#include "boards/zectrix-s3-epaper-4.2/custom_lcd_display.h"
#include "ui/renderers/rawdraw/settings_renderer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs.h"
#include "driver/gpio.h"
#include <atomic>

namespace extensions {
namespace {
constexpr char kTag[] = "extensions";
std::atomic<bool> busy{false};
bool NvsHealthy() {
    nvs_handle_t handle;
    if (nvs_open("lc_health", NVS_READWRITE, &handle) != ESP_OK) return false;
    uint32_t read = 0;
    const bool written = nvs_set_u32(handle, "probe", 0x4e3443) == ESP_OK && nvs_commit(handle) == ESP_OK;
    nvs_close(handle);
    if (!written || nvs_open("lc_health", NVS_READONLY, &handle) != ESP_OK) return false;
    const bool ok = nvs_get_u32(handle, "probe", &read) == ESP_OK && read == 0x4e3443;
    nvs_close(handle);
    return ok;
}
void HealthReady() {
    if (!lucidcairn::ota::PendingVerify()) return;
    auto& app = Application::GetInstance();
    auto* display = static_cast<CustomLcdDisplay*>(Board::GetInstance().GetDisplay());
    const bool healthy = lucidcairn::ota::LocalHealthPasses(
        gpio_get_level(VBAT_PWR_PIN) == 1, NvsHealthy(),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) >= 32768 &&
            heap_caps_check_integrity_all(false),
        display && display->GetFramebuffer() && app.GetRawDrawUiManager(),
        app.GetDeviceState() == kDeviceStateIdle);
    lucidcairn::ota::CompleteBootVerification(healthy);
}
#ifdef CONFIG_LUCIDCAIRN_OTA
std::string ReadString(nvs_handle_t handle, const char* key, size_t maximum) {
    size_t size = 0;
    if (nvs_get_str(handle, key, nullptr, &size) != ESP_OK || size <= 1 || size > maximum + 1) return {};
    std::string value(size, '\0');
    if (nvs_get_str(handle, key, value.data(), &size) != ESP_OK) return {};
    value.resize(size - 1);
    return value;
}
void CheckTask(void*) {
    lucidcairn::ota::Config config;
    nvs_handle_t handle;
    if (nvs_open("lc_ota", NVS_READONLY, &handle) == ESP_OK) {
        config.manifest_url = ReadString(handle, "manifest_url", 2048);
        config.device_token = ReadString(handle, "device_token", 512);
        nvs_close(handle);
    }
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        char id[13];
        snprintf(id, sizeof(id), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        config.device_id = id;
    }
    lucidcairn::ota::Power power;
    bool charging = false, discharging = false;
    power.known = Board::GetInstance().GetBatteryLevel(power.percent, charging, discharging);
    power.external = charging;  // Unknown/ambiguous full-charge state does not bypass the floor.
    const auto result = lucidcairn::ota::CheckAndInstall(config, power);
    // Never log credentials, manifest content or signed object URLs.
    ESP_LOGI(kTag, "Explicit OTA check result=%d", static_cast<int>(result));
    if (result == lucidcairn::ota::CheckResult::Installed) esp_restart();
    busy.store(false);
    vTaskDelete(nullptr);
}
void RequestCheck() {
    if (busy.exchange(true)) return;
    if (xTaskCreate(CheckTask, "lc_ota", 12288, nullptr, 3, nullptr) != pdPASS) {
        busy.store(false);
        ESP_LOGE(kTag, "Unable to start OTA check");
    }
}
#endif
struct Feature {
    const char* name;
    void (*boot)();
    void (*ready)();
    void (*settings)(std::vector<rawdraw::SettingsItemDef>&);
};
const Feature features[] = {
    {"recovery", lucidcairn::ota::BeginBootVerification, HealthReady, nullptr},
#ifdef CONFIG_LUCIDCAIRN_OTA
    {"manual_ota", nullptr, nullptr, [](auto& items) {
        items.push_back({"更新", "", nullptr, rawdraw::SettingsItemType::Section, false});
        items.push_back({"检查更新", "手动", nullptr, rawdraw::SettingsItemType::Action, false, RequestCheck});
    }},
#endif
};
}
void BeforeBoot() { for (const auto& feature : features) if (feature.boot) feature.boot(); }
void LocalReady() { for (const auto& feature : features) if (feature.ready) feature.ready(); }
void AddSettings(std::vector<rawdraw::SettingsItemDef>& items) {
    for (const auto& feature : features) if (feature.settings) feature.settings(items);
}
bool Busy() { return busy.load() || lucidcairn::ota::PendingVerify(); }
bool PendingVerify() { return lucidcairn::ota::PendingVerify(); }
void RejectPendingImage() { lucidcairn::ota::RejectPendingImage(); }
}
