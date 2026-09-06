# NOTE4C architecture and ownership

The product source of truth is `djjcool-pixel/youn-ink-fourcolor-firmware`, branch `2bp` plus reviewed extension changes. `note4c-ota-terminal` is a historical migration source, not a second hardware platform.

```text
ZECTRIX Wiki -> itopinion/2bp (official reference)
                   ^ shared ancestry
LazyYoun/2bp ------> product fork (reviewed upstream merges)
                       |
             upstream hardware + application
                       |
             main/extensions/feature_registry
                | local lifecycle | settings callbacks
             components/lucidcairn_ota
                | policy | bounded HTTPS | IDF app_update
             inactive app -> validate -> select boot
                -> pending verify -> local health -> accept / rollback
```

## Ownership map

| Surface | Owner | Extension rule |
|---|---|---|
| `firmware/main/boards/**`, BSP, GPIO mapping, SSD2683, power, charge, buttons, RTC, NFC, audio hardware | Upstream | No direct product edits; propose upstream fixes separately with device evidence |
| `firmware/main/rawdraw/**`, display, partition tables | Upstream | Reuse; CI checks byte-for-byte Git differences against audited baseline |
| `ui/renderers/rawdraw/PageRenderer`, `UiPageRegistry`, settings callbacks | Upstream | Existing seams; do not create another UI/page framework |
| `main/extensions/**` | Product | Small static registry and board/application adapters |
| `components/lucidcairn_ota/**` | Product | Provider-neutral policy and ESP-IDF adapters; no board headers |
| `main/main.cc`, `main/application.cc`, build glue | Shared integration boundary | Keep call sites small; examine every conflict on sync |
| OTA authentication/manifest/artifact hosting | Separate service | No provider credentials or cloud SDK in firmware; no production cutover here |

## Existing seam audit

The checked-out `2bp` application initializes the gallery/settings/transfer flow. Although OTA dialogs and IDF OTA dependencies remain, no active `esp_ota_begin`, `esp_https_ota` download, or pending-image acceptance implementation existed in the baseline. There is no working OTA implementation to wrap. The new module reuses ESP-IDF `esp_http_client`, CA bundle, `esp_ota_begin/write/end`, partition descriptions, `esp_ota_set_boot_partition`, and rollback APIs. It does not copy the legacy Arduino `Update` layer or reimplement flash/image validation.

`Feature` is a fixed array of boot, local-ready and settings hooks. Recovery is always present; manual OTA is selected with `CONFIG_LUCIDCAIRN_OTA`. Adding a feature means one source module and one entry, not reflection, runtime loading, an event bus, or a new scheduler. New UI content should implement the upstream `PageRenderer` and use upstream theme tokens. No custom page is introduced just to demonstrate a plugin.

## Boot and update lifecycle

`main` starts a 90-second local verification deadline before reset workarounds or NVS repair. Only pending images suppress the upstream software-reset sleep bounce. Pending-image NVS repair failures request rollback instead of erasing configuration. After application initialization, the adapter checks the power-latch input using the upstream GPIO constant, an NVS round-trip, internal heap capacity/integrity, display/framebuffer/UI presence and nonfatal application state. It never requires Wi-Fi, DNS, time sync or an external service.

The display probe checks allocation/initialization presence, **not physical panel refresh**; the BSP has no reliable high-level panel self-test result. The deadline is an IDF timer, not an independent hardware supervisor. These limits require real-device acceptance, not invented success evidence.

The manual OTA worker serializes requests, reads separately provisioned NVS credentials, snapshots upstream battery status and uses bounded HTTPS reads. Application-triggered sleep and restart are gated while an update or boot verification is active. Physical power loss and upstream board shutdown remain possible and must be handled by the IDF A/B recovery mechanism; no power/button implementation was changed.

Before writing, the client requires the audited upstream A/B + NVS + otadata + assets layout, an unstaged current boot slot, a newer version, valid manifest identity, and known battery state. It writes the inactive app partition only. It verifies exact wire byte count and SHA-256 before `esp_ota_end`, then verifies the image descriptor before selecting boot. Any earlier failure leaves the boot selection untouched. IDF may still fail a final otadata write; this is surfaced as a failed update and must be included in power-interruption tests.

## Compatibility and limitations

- Upstream app slots are `0x3f0000`, at `0x20000` / `0x410000`; assets remain at `0x800000`. Legacy 3 MiB slots and its NVS layout are incompatible. No partition table or bootloader migration is performed.
- A running application cannot prove which bootloader was flashed merely by compiling rollback support. Later commissioning must verify the installed bootloader and recovery slot before enabling OTA.
- Manifest schema 1 adds explicit board/project/layout identity to legacy concepts; the legacy service does not satisfy it unchanged.
- TLS authenticates the manifest service, SHA-256 binds the downloaded bytes to that manifest. This is not a signed-manifest/offline trust system. eFuse, secure boot, encryption and anti-rollback fuses remain outside scope.
- No credentials, production URLs, stable tags, device deployment or cloud changes are part of this migration.
