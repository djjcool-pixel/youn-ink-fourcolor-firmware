#!/usr/bin/env python3
"""Remove machine-specific paths from the component-manager lock, preserving pins."""
from pathlib import Path
import yaml

def normalize(firmware: Path):
    lock = firmware / "dependencies.lock"
    data = yaml.safe_load(lock.read_text())
    for component in data["dependencies"].values():
        source = component.get("source", {})
        if source.get("type") != "local":
            continue
        path = Path(source["path"])
        if path.is_absolute():
            source["path"] = path.resolve().relative_to(firmware.resolve()).as_posix()
        if not source["path"].startswith("components/"):
            raise ValueError("Unexpected local dependency outside firmware/components")
    lock.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")

if __name__ == "__main__":
    normalize(Path(__file__).resolve().parents[1] / "firmware")
