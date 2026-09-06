# NOTE4C migration development report

Date: 2026-09-06. Repository-only migration; no real-device or production-release gate executed.

## Audit and implementation

- Verified the official Wiki entry, itopinion `4a46aa9`, LazyYoun `51812e4`, the product fork at the same `51812e4`, and legacy `dca6dc8`. The official pointer is seven commits behind the code upstream; no upstream reset was made.
- Established main-firmware ownership, two-source sync guidance, a pinned audit record, Agent entry points, architecture, OTA contract, acceptance checklist and attribution. Preserved the original README as a clearly historical upstream snapshot.
- Reused upstream UI seams with a small compile-time lifecycle/settings registry. The existing active gallery application did not contain a connected OTA writer; the new component uses ESP-IDF HTTP/TLS and app_update primitives.
- Added strict provider-neutral manifest/device-token handling, bounded inactive-slot transactions, size/hash/image identity gates, local pending-image health, rollback deadline and guards against early pending-image sleep/NVS repair.
- Kept BSP, SSD2683, power/buttons, RawDraw, display and partition sources unchanged. Added a scoped CMake alias for the vendored camera component's IDF 6 enum name when compiling with the fixed IDF 5.5.2 CI baseline, plus Windows response files for long compiler commands.
- Added host policy/transaction and boot-health fault tests, boundary checks and a build-only GitHub workflow for OTA off/on. No service endpoint, credential, stable tag or distribution artifact is provisioned.
- Legacy status PR: https://github.com/djjcool-pixel/note4c-ota-terminal/pull/3 (documentation only, no archive/deletion).

## Validation status

Host policy/transaction assertions, boot-health assertions and upstream boundary checks have run locally. Complete final build/CI evidence is recorded below once the exact PR revision finishes verification; intermediate build preparation must not be treated as a successful device test.

| Check | Status |
|---|---|
| Strict OTA manifest/policy/transaction host suite | Initial suite passed; final revision verification in progress |
| Pending boot acceptance, failure, timeout and stale-timer host suite | Passed locally; final CI pending |
| Protected upstream source/license diff | Passed |
| ESP-IDF OTA disabled/enabled full builds | In progress |
| GitHub CI at final PR head | Pending |
| Flash / production OTA / stable tag / eFuse/security changes | Not executed |
| Physical power/display/buttons/sleep and A/B rollback | Not executed; see HARDWARE_ACCEPTANCE.md |

## Remaining device/service work

Verify actual installed bootloader/partition layout and known-good fallback before any future installation. Exercise both A/B directions, failed pending boot, power/network interruptions, local health while offline, physical panel refresh and manual sleep/button interactions. A framebuffer pointer is not proof of a functioning panel. Provision separately scoped device credentials and implement schema 1 on the selected provider-neutral service before enabling the manual OTA action. None of these deployment/device actions is implied by this PR.
