# NOTE4C · Youn Ink + LucidCairn extensions

本仓库 `djjcool-pixel/youn-ink-fourcolor-firmware` 是我们的 **NOTE4C 主固件仓库**。基于 LazyYoun 的 `2bp` 四色相册固件，复用 ZECTRIX 上游平台，在应用边界增加少量编译期扩展。

**Upstream owns the hardware platform; we own product differentiation.** BSP、SSD2683、RawDraw、电源、按键、音频及分区表保持上游实现。旧 `djjcool-pixel/note4c-ota-terminal` 仅作 legacy / migration source，历史记录和恢复资料继续保留。

## 当前可用范围

- 上游：NOTE4C ESP32-S3 N16R8、400×300 BWRY 四色相册、AP/LAN 传图、设置和 Wi-Fi。README 旧版对语音助手/服务端/OTA 的描述不能当作当前已接通功能；原文保存在 [上游 README 快照](docs/UPSTREAM_README.md)。
- 扩展：固定编译期 feature registry，复用现有 `PageRenderer` 和设置项回调，不引入动态插件系统。
- Recovery：pending verify 镜像在本地 power / NVS / heap / display / application 检查后确认；失败或启动超时请求 IDF rollback。pending 镜像跳过软件复位深睡眠跳转，NVS 初始化异常时先回滚。
- OTA：独立 ESP-IDF component，HTTPS、受限单设备 token、严格 manifest、inactive slot、精确 size + SHA-256、ESP 镜像/版本检查后才选 boot slot。**默认关闭下载入口**；开启后也只提供手动检查，不自动下载。
- CI：主机故障注入、硬件边界检查、ESP-IDF 固件构建（OTA off/on），不创建 release、不发布可被设备消费的版本。

仓库代码与 CI 不代表你的设备已升级。本次迁移以你已刷入的官方固件为基线，**未进行真机刷写、生产 OTA 或稳定发布**。当前实际验证证据见 [迁移开发报告](docs/MIGRATION_REPORT.md)。

## Agent / 开发者入口

1. [AGENTS.md](AGENTS.md)：修改边界与验收命令。
2. [ARCHITECTURE.md](ARCHITECTURE.md)：所有权、调用链、扩展接入点。
3. [UPSTREAM_SYNC.md](docs/UPSTREAM_SYNC.md) 与 [upstream-lock.json](docs/upstream-lock.json)：来源、固定审计点、同步步骤。
4. [OTA_CONTRACT.md](docs/OTA_CONTRACT.md)：协议、凭据、兼容性及失败语义。
5. [HARDWARE_ACCEPTANCE.md](docs/HARDWARE_ACCEPTANCE.md)：后续真机验收条件。

## 构建与测试

使用 ESP-IDF **v5.5.2** shell，Python 3.10+；官方上游 README 提到的开发环境为 IDF 6.0，本分支的可重复 CI 基线单独固定，不声称兼容所有 `>=5.4` 版本。

```sh
python scripts/build_firmware.py --ota off
python scripts/build_firmware.py --ota on
python scripts/check_boundaries.py
cmake -S tests/ota -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel 2
ctest --test-dir build-host --output-on-failure -V
```

两种配置使用独立构建目录；构建脚本检查实际 sdkconfig 和 app 大小，输出 `build-evidence.json`。未提供真实 endpoint/token；不要把凭据写进配置默认值或源码。开发构建沿用上游版本元数据，不能把它当作新的稳定版本发布。

## 来源与许可

[ZECTRIX NOTE4C 指南](https://wiki.zectrix.com/zh/hardware/note4c/quick-start) 和 [官方开源范围](https://wiki.zectrix.com/zh/software/opensource) 指向 [itopinion/2bp](https://github.com/itopinion/youn-ink-fourcolor-firmware/tree/2bp)，其源自 [LazyYoun/2bp](https://github.com/LazyYoun/youn-ink-fourcolor-firmware/tree/2bp)。2026-09-06 核对时，当前 fork 与 LazyYoun 同在 `51812e4`，比 itopinion `4a46aa9` 多 7 个提交。

保留原 [根 MIT License](LICENSE)、[firmware MIT License](firmware/LICENSE) 与组件许可证；新增扩展采用 MIT。详见 [第三方归属](THIRD_PARTY_NOTICES.md)。
