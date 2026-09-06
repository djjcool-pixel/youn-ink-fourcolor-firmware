// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include "esp_err.h"

namespace lucidcairn::ota {
// No credentials or scheduler are created by this component.
struct Config { std::string manifest_url, device_token, device_id; };
struct Power { bool known = false; int percent = 0; bool external = false; };
enum class CheckResult { Installed, NoUpdate, Disabled, Rejected, Failed };
CheckResult CheckAndInstall(const Config& config, Power power);
bool PendingVerify();
// Call very early, before any reset, sleep, networking or NVS repair.
void BeginBootVerification();
// Local checks only; a failed check rolls back, or aborts if no rollback is possible.
void CompleteBootVerification(bool local_health_passed);
void RejectPendingImage();
}  // namespace lucidcairn::ota
