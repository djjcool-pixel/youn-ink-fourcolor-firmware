# Agent entry point

Read `README.md`, `ARCHITECTURE.md`, `docs/UPSTREAM_SYNC.md`, then the touched contract and current source. The current user request defines authorization. Complete authorized reversible code/docs/tests/branches/PR work autonomously; do not ask for confirmation between routine steps.

## Ownership and scope

- This is the NOTE4C main firmware repository. Upstream owns the hardware platform; we own product differentiation.
- Keep BSP, SSD2683, GPIO17/power, battery/charge, buttons, RawDraw and partition tables unchanged for product extensions. `scripts/check_boundaries.py` enforces the recorded source baseline. Never update the lock just to hide an unintended hardware diff.
- Reuse `PageRenderer`, `UiPageRegistry`, settings callbacks and the fixed registry in `main/extensions`. No dynamic plugin loader or parallel UI framework.
- Keep generic OTA policy inside `firmware/components/lucidcairn_ota`, hardware observations in the small application adapter. Preserve failure cleanup, inactive-slot writes, exact size/hash checks, pending verification and rollback.
- Manifest URLs and device tokens are separately provisioned. Never commit credentials, NVS dumps, signed artifact URLs, local sdkconfig, generated binaries or managed components. No cloud-provider SDK/credentials in firmware.
- Internet/Wi-Fi/DNS/cloud availability is never local firmware health. Display/framebuffer presence is not proof of physical panel function.
- The private legacy repository has no root license at the audited commit. Do not bulk-copy its source or private run reports into this public repository. The migration implements its design concepts afresh using ESP-IDF and records provenance.

## Validation

Run the README host checks and both `scripts/build_firmware.py --ota off/on` profiles. Keep rollback and boot validation enabled. Preserve `0x3f0000` slot bounds. Record actual image size, commit and CI results in `docs/MIGRATION_REPORT.md`; distinguish compilation from physical tests. CI is build-only.

On upstream sync, fetch both named remotes and review ancestry before changing code. Update `docs/upstream-lock.json` only after reviewing incoming upstream changes and rerunning checks. Preserve original licenses and third-party notices. Never reset/rebase a published product branch or force-push an upstream sync.

## Separate hardware/release authorization

Ordinary repository maintenance does not authorize real flashing/erase, partition/bootloader/NVS writes to a device, creating stable tags, production OTA, credential rotation, cloud cutover, deleting recovery backups, archiving legacy, or irreversible eFuse/secure-boot/flash-encryption operations. Prepare a concrete reviewable result and the hardware acceptance checklist; carry on with all independent repository work. Honor explicit session authorization when present.
