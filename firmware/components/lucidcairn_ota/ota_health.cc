// SPDX-License-Identifier: MIT
#include "lucidcairn_ota.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <cstdlib>
#include <atomic>

namespace lucidcairn::ota {
namespace {
esp_timer_handle_t deadline = nullptr;
enum class Verification { Idle, Pending, Accepting, Failed };
std::atomic<Verification> verification{Verification::Idle};
constexpr char kTag[] = "ota_health";
[[noreturn]] void Rollback() {
    ESP_LOGE(kTag, "Local boot verification failed; requesting rollback");
    const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
    // No fallback, or metadata failure: do not mark valid, erase NVS or continue.
    ESP_LOGE(kTag, "Rollback could not restart: %s", esp_err_to_name(err));
    std::abort();
}
}
bool PendingVerify() {
    const auto* running = esp_ota_get_running_partition();
    if (!running) std::abort();
    esp_ota_img_states_t state;
    const esp_err_t err = esp_ota_get_state_partition(running, &state);
    // A first USB installation may have no otadata record yet.
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_NOT_SUPPORTED) return false;
    if (err != ESP_OK) std::abort();
    return state == ESP_OTA_IMG_PENDING_VERIFY;
}
void BeginBootVerification() {
    if (!PendingVerify() || deadline) return;
    esp_timer_create_args_t args = {};
    verification.store(Verification::Pending);
    args.callback = [](void*) {
        auto expected = Verification::Pending;
        if (verification.compare_exchange_strong(expected, Verification::Failed)) Rollback();
    };
    args.name = "ota_local_health";
    ESP_ERROR_CHECK(esp_timer_create(&args, &deadline));
    ESP_ERROR_CHECK(esp_timer_start_once(deadline, 90LL * 1000 * 1000));
}
void CompleteBootVerification(bool healthy) {
    if (!PendingVerify()) return;
    if (!healthy) Rollback();
    auto expected = Verification::Pending;
    if (!verification.compare_exchange_strong(expected, Verification::Accepting)) Rollback();
    ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
    if (deadline) {
        esp_timer_stop(deadline);
        esp_timer_delete(deadline);
        deadline = nullptr;
    }
    ESP_LOGI(kTag, "Image accepted by local power/NVS/heap/display/application checks");
    verification.store(Verification::Idle);
}
void RejectPendingImage() { if (PendingVerify()) Rollback(); }
}  // namespace lucidcairn::ota
