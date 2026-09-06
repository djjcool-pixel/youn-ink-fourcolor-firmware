#!/usr/bin/env python3
"""Build only: never flash, publish, provision credentials or create releases."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from normalize_lock import normalize

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--ota", choices=["off", "on"], default="off")
args = parser.parse_args()
firmware = root / "firmware"
build = firmware / ("build-ci-" + args.ota)
defaults = ["sdkconfig.defaults", "sdkconfig.defaults.esp32s3", "sdkconfig.defaults.lucidcairn"]
if args.ota == "on":
    defaults.append("sdkconfig.defaults.ota-test")
idf = Path(os.environ["IDF_PATH"]) / "tools/idf.py"
subprocess.run([sys.executable, str(idf), "-B", str(build), "-DIDF_TARGET=esp32s3",
                "-DSDKCONFIG=" + str(build / "sdkconfig"),
                "-DSDKCONFIG_DEFAULTS=" + ";".join(defaults), "build"], cwd=firmware, check=True)
normalize(firmware)
image = build / "xiaozhi.bin"
data = image.read_bytes()
if not 0 < len(data) <= 0x3F0000:
    raise SystemExit("Image exceeds the unchanged upstream OTA slot")
config = (build / "sdkconfig").read_text()
for required in ["CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y", "CONFIG_ZECTRIX_EPD_PANEL_4COLOR_SSD2683=y"]:
    if required not in config:
        raise SystemExit("Missing safety config: " + required)
for forbidden in ["CONFIG_BOOTLOADER_SKIP_VALIDATE_ALWAYS=y", "CONFIG_SECURE_BOOT=y", "CONFIG_SECURE_FLASH_ENC_ENABLED=y"]:
    if forbidden in config:
        raise SystemExit("Unexpected boot/security configuration: " + forbidden)
if ("CONFIG_LUCIDCAIRN_OTA=y" in config) != (args.ota == "on"):
    raise SystemExit("Wrong extension selection")
evidence = {"ota": args.ota, "bytes": len(data), "slot_bytes": 0x3F0000,
            "sha256": hashlib.sha256(data).hexdigest(), "hardware_verified": False}
(build / "build-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
print(json.dumps(evidence))
