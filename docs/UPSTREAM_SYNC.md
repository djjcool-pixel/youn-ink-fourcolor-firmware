# Upstream source audit and synchronization

Audited 2026-09-06. Source facts are pinned in `upstream-lock.json`; GitHub branch tips may change after this date.

| Repository | Branch | Audited commit | Role |
|---|---|---|---|
| `itopinion/youn-ink-fourcolor-firmware` | `2bp` | `4a46aa936644f1ee97e1804512260a6738fa4345` | Official Wiki maintenance entry |
| `LazyYoun/youn-ink-fourcolor-firmware` | `2bp` | `51812e4ab3fa80ba7a5a5a274635ca2cf3901a25` | Current code upstream |
| `djjcool-pixel/youn-ink-fourcolor-firmware` | `2bp` | `51812e4ab3fa80ba7a5a5a274635ca2cf3901a25` | Product source of truth before migration |
| `djjcool-pixel/note4c-ota-terminal` | `main` | `dca6dc8` | Private legacy / migration source, remains unarchived |

`git rev-list --left-right --count zectrix/2bp...upstream/2bp` returned `0 7`: itopinion is an ancestor, not a competing newer hardware branch. Retain the existing seven commits, including the SSD2683 framebuffer-diff and Wi-Fi IP-wait fixes. Do not reset to the older official pointer.

The [NOTE4C guide](https://wiki.zectrix.com/zh/hardware/note4c/quick-start) and [official open-source page](https://wiki.zectrix.com/zh/software/opensource) distinguish public source from commercial cloud/production firmware. A user's flashed official binary is not proven bit-identical to any source checkout by these links.

## Remotes

```sh
git remote add upstream https://github.com/LazyYoun/youn-ink-fourcolor-firmware.git
git remote add zectrix https://github.com/itopinion/youn-ink-fourcolor-firmware.git
git fetch upstream 2bp
git fetch zectrix 2bp
git log --oneline --left-right upstream/2bp...zectrix/2bp
```

Use `git remote set-url` only when an existing named remote is demonstrably incorrect. `origin` remains the djjcool-pixel fork. Never push product changes to either upstream remote.

## Reviewed merge workflow

1. Start a clean topic branch from current `origin/2bp`. Inspect the new upstream range relative to the lock, including licenses, manifests, build tools, board files and partition tables.
2. If LazyYoun advanced, merge `upstream/2bp` into that topic branch with history preserved. Review the small integration points: `main.cc`, `application.cc`, CMake and dependency resolution. Do not replace the entire application tree with legacy files.
3. If only itopinion advanced, review its ancestry and changes independently. Merge/cherry-pick only an understood fix; a changed Wiki pointer alone is not a reason to switch baselines. Document the decision.
4. Update the lock to the upstream commit actually integrated; document justified upstream hardware changes. The boundary check must then compare against that newly reviewed baseline. Reconcile all original third-party notices.
5. Resolve dependencies in the pinned IDF environment and commit the portable lock. Run host faults, OTA off/on builds and boundary checks. A partition/bootloader/hardware change requires a separately authorized device plan before any installation.
6. Open a product-fork PR with old/new SHAs, validation and required device checks. No force push, stable tag, automatic merge or device update is part of sync.

## Build baseline

The upstream manifest accepts `>=5.4.0`; its README names a local IDF 6.0 directory and there was no GitHub build CI. This fork fixes CI at ESP-IDF v5.5.2 with resolved component versions. Historical `firmware/.cnb.yml` belongs to upstream infrastructure and is not a GitHub workflow or an approved product deployment path. Do not wire it to this fork's production infrastructure.
