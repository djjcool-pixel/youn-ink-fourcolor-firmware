# Provider-neutral OTA contract, schema 1

This is the new firmware contract, not evidence of a live OTA service. It deliberately rejects the legacy API response unless that service adds the explicit schema and compatibility fields. No backend/provider migration occurs here.

## Request and credentials

Only an explicit settings action in an OTA-enabled build starts a check. Read NVS namespace `lc_ota`, keys `manifest_url` (full HTTPS URL, max 2048 ASCII bytes) and `device_token` (max 512 characters, `[A-Za-z0-9._~-]`). Missing/invalid values leave OTA disabled. No credentials are embedded by CI. A future authorized provisioning process supplies these per-device values; no provisioning endpoint or UI is introduced here.

The manifest request includes `Authorization: Bearer <device token>` and `X-Device-ID: <12 lowercase hex Wi-Fi MAC>`. The service must bind each token to one registered device ID and restrict it to firmware reads; a MAC header alone is not authentication. No GitHub, COS or other provider credential belongs on a device. Tokens remain plaintext in NVS until a separate storage-security design is approved; no encryption/eFuse changes occur here.

HTTP 204 means no update. HTTP 200 must contain a complete JSON object, <=8192 bytes, with no duplicate keys, embedded NUL, trailing JSON or invalid field types. Redirects are rejected. HTTPS CA verification stays enabled. Content transformations are rejected (`Accept-Encoding: identity`).

```json
{
  "schema_version": 1,
  "board": "note4c",
  "project": "xiaozhi",
  "layout": "note4c-16m-v2",
  "version": "6.5.10",
  "download_url": "https://firmware.example.org/signed-app.bin?signature=example",
  "size": 1234567,
  "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "minimum_battery_percent": 30
}
```

The example is illustrative: its size/hash do not identify an actual release. Hash is 64 hexadecimal digits; size is a positive integer <= `0x3f0000`. Optional minimum battery defaults to 30 and must be an integer 30..100. Unknown battery status blocks the update; a confirmed charging state may bypass the percentage threshold.

Versions are canonical numeric `MAJOR.MINOR.PATCH` with optional lowercase `v`, each part <= uint32, no leading zeros, prerelease suffix, build metadata or trailing text. Candidate must be strictly greater than the currently running image version. Invalid current versions also block updates. The downloaded app descriptor must match the candidate version (without `v`) and project `xiaozhi`.

## Download and transaction

A same-origin download URL receives the device token and device ID. A different HTTPS origin is treated as an already signed URL and receives neither. Origin compares lowercase DNS/IPv4 host and normalized port; credentials in URLs, IPv6 literals, fragment, whitespace, backslash and ambiguous authorities are rejected. All redirects are rejected so credentials never travel to a redirect target. The server must issue a direct URL whose signature lasts for the bounded download window.

Transport read timeout is 15 seconds; each HTTP operation is bounded by a 180-second read deadline plus a possible final read timeout. A manifest is bounded before allocation grows; a binary chunk that would exceed declared size is rejected before writing. Content-Length, when not chunked, must match the manifest. Chunked transfers require complete EOF and the same exact byte count/hash checks. There is no partial resumption.

```text
validate current layout / current boot slot / no pending image
 -> fetch and validate manifest / newer version / battery
 -> open HTTPS download, verify status and response headers
 -> esp_ota_begin(inactive app)
 -> hash exact received bytes, esp_ota_write
 -> complete EOF + exact size + SHA-256
 -> esp_ota_end (ESP image validation; no boot selection)
 -> app descriptor identity matches
 -> esp_ota_set_boot_partition
 -> reboot -> bootloader PENDING_VERIFY
 -> local checks pass: mark valid; otherwise rollback
```

All failed transactions clean up HTTP and active OTA handles. No boot selection before integrity and image validation. No filesystem, assets, partition table, bootloader or NVS image is accepted as an OTA payload. Layout comparison is pinned to upstream NVS `0x9000/0x4000`, otadata `0xd000/0x2000`, apps `0x20000` and `0x410000` of `0x3f0000`, assets `0x800000/0x800000`.

## Recovery semantics

Local acceptance requires power hold, NVS read/write, internal heap >=32 KiB and integrity, display framebuffer/UI presence, and application idle state. Network readiness is absent from this decision. A pending boot cannot erase NVS as a repair fallback. Its 90-second timer requests rollback if local startup stalls; if the scheduler itself stalls, recovery depends on watchdog/reset and the rollback-enabled bootloader. If IDF cannot roll back, the application aborts and never falsely accepts the image.

Known-good normal boots do not perform the NVS health write. Boot verification is present even when the download feature is compiled out. Removing the feature therefore does not remove acceptance/rollback handling for images installed by an already-enabled predecessor.

Hardware validation must establish the actual installed bootloader supports rollback and a valid fallback exists. An application-only OTA cannot retrofit a bootloader, migrate old partition offsets, or turn a runtime code check into physical panel proof. See `HARDWARE_ACCEPTANCE.md`.
