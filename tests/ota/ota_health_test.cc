// SPDX-License-Identifier: MIT
#include "lucidcairn_ota.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include <cstdlib>
#include <iostream>
using namespace lucidcairn::ota;
struct Rollback {};
static esp_partition_t partition;
static esp_ota_img_states_t state = ESP_OTA_IMG_VALID;
static int accepted = 0, rollbacks = 0, timers = 0, stopped = 0, state_error = ESP_OK;
static void (*timeout_callback)(void*) = nullptr;
static int checks = 0;
#define CHECK(test) do { ++checks; if (!(test)) { std::cerr << "failed line " << __LINE__ << '\n'; std::exit(1); } } while (0)
const esp_partition_t* esp_ota_get_running_partition() { return &partition; }
esp_err_t esp_ota_get_state_partition(const esp_partition_t*, esp_ota_img_states_t* out) { *out = state; return state_error; }
esp_err_t esp_ota_mark_app_valid_cancel_rollback() { ++accepted; state = ESP_OTA_IMG_VALID; return ESP_OK; }
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot() { ++rollbacks; throw Rollback{}; }
esp_err_t esp_timer_create(const esp_timer_create_args_t* args, esp_timer_handle_t* handle) {
    timeout_callback = args->callback; *handle = &partition; ++timers; return ESP_OK;
}
esp_err_t esp_timer_start_once(esp_timer_handle_t, int64_t delay) { CHECK(delay == 90000000); return ESP_OK; }
esp_err_t esp_timer_stop(esp_timer_handle_t) { ++stopped; return ESP_OK; }
esp_err_t esp_timer_delete(esp_timer_handle_t) { return ESP_OK; }
int main() {
    BeginBootVerification(); CompleteBootVerification(false); RejectPendingImage();
    CHECK(accepted == 0 && rollbacks == 0 && timers == 0);
    state_error = ESP_ERR_NOT_FOUND; CHECK(!PendingVerify());
    state_error = ESP_ERR_NOT_SUPPORTED; CHECK(!PendingVerify()); state_error = ESP_OK;
    state = ESP_OTA_IMG_PENDING_VERIFY;
    CHECK(PendingVerify()); BeginBootVerification(); BeginBootVerification();
    CHECK(timers == 1);
    CompleteBootVerification(true);
    CHECK(accepted == 1 && stopped == 1 && !PendingVerify());
    timeout_callback(nullptr); CHECK(rollbacks == 0);
    CompleteBootVerification(true); CHECK(accepted == 1);
    state = ESP_OTA_IMG_PENDING_VERIFY; BeginBootVerification();
    try { CompleteBootVerification(false); CHECK(false); } catch (const Rollback&) {}
    CHECK(rollbacks == 1 && accepted == 1);
    try { RejectPendingImage(); CHECK(false); } catch (const Rollback&) {}
    CHECK(rollbacks == 2);
    try { timeout_callback(nullptr); CHECK(false); } catch (const Rollback&) {}
    CHECK(rollbacks == 3 && accepted == 1);
    std::cout << checks << " boot health state checks passed\n";
}
