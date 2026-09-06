# Third-party attribution and migration provenance

The original license files and source headers are preserved, not replaced by a new blanket license.

| Material | Origin / notice |
|---|---|
| Repository baseline | LazyYoun/youn-ink-fourcolor-firmware `51812e4ab3fa80ba7a5a5a274635ca2cf3901a25`, MIT, copyright 2026 macheng2017; see root `LICENSE` |
| Firmware lineage | Existing `firmware/LICENSE`: MIT, copyright 2025 Shenzhen Xinzhi Future Technology Co., Ltd. and Project Contributors |
| Official reference | itopinion/youn-ink-fourcolor-firmware `4a46aa936644f1ee97e1804512260a6738fa4345`, as linked by ZECTRIX Wiki; original notices preserved |
| ESP-IDF | Espressif Systems, v5.5.2 CI baseline; ESP-IDF is primarily Apache-2.0 with separately licensed included components. Retain the resolved SDK/component notices in distributions |
| cJSON | Dave Gamble and cJSON contributors, MIT; host tests pin v1.7.19 commit `c859b25da02955fef659d658b8f324b5cde87be3`; firmware uses the component manager's recorded cJSON version |
| Existing vendored drivers/fonts/assets | Keep source/component licenses, metadata and original copyright headers in place. Root MIT does not override per-component terms. No new fonts/assets copied by this migration |
| New product code | `lucidcairn_ota`, `main/extensions`, migration tests/scripts/docs: MIT, copyright 2026 djjcool-pixel; component includes the full new-code license |

Recovery-first design provenance: `djjcool-pixel/note4c-ota-terminal`, audited `dca6dc8`, especially its OTA manager and local-health design. It is a private historical repository with no root license at that commit. This migration implements the useful protocol/recovery concepts anew using ESP-IDF; it does not bulk-copy Arduino firmware, third-party driver code, private run logs, credentials or factory backups, and does not assert a new license over the legacy history.

The portable dependency lock records versions/hashes, not a complete legal review or SBOM. Future artifact distribution must include the resolved third-party license notices, including upstream fonts/assets and SDK dependencies. No stable distribution is produced by the migration.
