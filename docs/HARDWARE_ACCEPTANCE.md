# Deferred NOTE4C device acceptance

No item below was executed by the architecture migration. The user's reported official firmware installation is the current device baseline; its exact binary, partition table and bootloader have not been inspected by this task.

Before a separately authorized installation, record the actual board/model, running firmware, full partition table and bootloader capabilities. Preserve a verified complete 16 MiB backup in two locations outside Git and prove a USB recovery route. Never use legacy app/partition files just because the MCU and display model match.

| Gate | Required evidence |
|---|---|
| Upstream baseline | Official firmware still boots; four colors, gallery/transfer, buttons, battery, power hold and sleep behave normally |
| Layout/rollback readiness | Installed A/B offsets and sizes match this source; bootloader rollback support and a bootable known-good fallback verified |
| New firmware boot | No pending-image deep-sleep bounce; no unexpected NVS erase; GPIO17 high; display and UI usable; local health accepted |
| Offline boot health | Wi-Fi unavailable, DNS unavailable and OTA service unavailable still permit local acceptance |
| Display limitation | Physical refresh and panel-busy failure cases assessed; framebuffer allocation alone cannot detect a disconnected/broken panel |
| Happy-path A/B | Both directions A→B and B→A, correct partition written, image hash, first-boot state, mark-valid observed |
| Invalid download | Wrong hash/size/type/layout/version, expired URL, 401/403, redirects, unknown/low battery; boot slot unchanged |
| Interrupted transfer | Network cut and controlled power loss during erase/write/finalization; previous image recovers |
| Failed pending image | Failing local check, startup timeout, crash, watchdog, power loss before acceptance; fallback observed without NVS loss |
| User interactions | Buttons, explicit sleep/restart and Wi-Fi changes during OTA; no application sleep while busy; physical power loss recovery |
| Recovery failure | Missing fallback/otadata failure produces a diagnosable recoverable state; never claims a healthy image |
| Credentials | Per-device token restriction and revocation tested; signed cross-origin download does not receive the device token |

Record exact image SHA-256, build commit/config, observed serial logs and outcomes. CI build or host fakes do not satisfy these gates. Stable release/tag, production OTA, flashing and irreversible security operations require their own explicit authorization; they are not implied by this checklist.
