#!/usr/bin/env python3
"""Verify the audited hardware/platform sources and license notices stayed intact."""
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
record = json.loads((root / "docs/upstream-lock.json").read_text())
baseline = record["code_upstream"]["commit"]
protected = ["firmware/main/boards", "firmware/main/rawdraw", "firmware/main/display",
             "firmware/partitions", "firmware/partitions.csv", "LICENSE", "firmware/LICENSE"]
subprocess.run(["git", "diff", "--exit-code", baseline, "--", *protected], cwd=root, check=True)
subprocess.run(["git", "diff", "--check"], cwd=root, check=True)
for required in ["ARCHITECTURE.md", "AGENTS.md", "THIRD_PARTY_NOTICES.md", "docs/UPSTREAM_SYNC.md",
                 "docs/OTA_CONTRACT.md", "docs/HARDWARE_ACCEPTANCE.md"]:
    if not (root / required).is_file():
        raise SystemExit("Missing entry point: " + required)
print("Upstream hardware, partition tables and original license notices unchanged")
