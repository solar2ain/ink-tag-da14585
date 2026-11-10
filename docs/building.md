# 源码构建

中文 | [English](building.en.md)

本文说明如何从源码编译 DA14585 App、Recovery、启动加载器和 GUI 资源。构建入口 [build.ps1](../build.ps1)、[Makefile](../Makefile) 和 [Makefile.signed](../Makefile.signed) 均位于仓库根目录。

## 依赖与版本

| 依赖 | 实际要求 |
| --- | --- |
| 主机 | Windows、PowerShell、Python 3.12 |
| 构建入口 | GNU Make、PowerShell 7（`pwsh`） |
| 编译器 | Keil uVision，Arm Compiler 6.24；需自行取得有效许可 |
| 外部 SDK | Renesas/Dialog DA145xx SDK 6.0.24.1464；按供应商条款自行获取 |
| 构建脚本 Python 依赖 | 默认构建只需标准库；传入签名 App 时另需 `cryptography` 做验签 |
| 离线 C 检查 | GCC 或 WSL GCC；可选 ASan/UBSan |

SDK、编译器和烧录工具需按各自条款单独取得。SDK 库存记录用于核对文件完整性。

## 构建步骤

在快照根目录执行；路径是示例，请改为已合法获取的实际目录。只对已核来源的 SDK 建立一次库存；已有库存／工作副本不覆盖，未知变化停止。

```powershell
python tools/sdk/da_sdk_workspace.py preserve-base --source 'D:/SDK/DA145xx_SDK/6.0.24.1464' --sdk-root 'D:/InkTagSDK'
python tools/sdk/da_sdk_workspace.py verify --sdk-root 'D:/InkTagSDK'
make SDK_ROOT=D:/InkTagSDK
```

Keil 不在默认 `%LOCALAPPDATA%/Keil_v5` 时，加 `-KeilRoot 'C:/Keil_v5'`。保留原 SDK 目录、`da6-base.json` 和所有依赖；不自动下载或放宽库存校验。两种策略分别使用 `.local/da-sdk6-optional` 和 `.local/da-sdk6-required`，默认增量 `-b`，不共享对象缓存。

输出在 `.local/builds/optional/<build-id>/` 或 `.local/builds/required/<build-id>/`，保留 App、Recovery、secondary loader raw BIN、外置 GUI 资源及原生证明。默认 `make` 同时在该次构建的 `packages/` 下生成：

| 文件 | 用途 |
| --- | --- |
| `InkTag-da14585_aesl-ota.img` | 明确标记为未签名、带 SHA-256 校验的 App OTA |
| `InkTag-da14585_aesl-recovery.img` | 本次构建 Recovery 的原生容器 |
| `InkTag-da14585_aesl-full.bin` | 256 KiB 完整固件模板，包含配对的 App、Recovery、loader、GUI 和初始数据 |
| `package-manifest.json` | 策略、组件大小／SHA-256、原生检查及输入证明绑定 |

也可执行 `make full SDK_ROOT=D:/InkTagSDK`。每次使用独立构建目录，不覆盖旧包。raw BIN 与打包结果分开，烧录使用 Full 文件；App OTA 使用 `.img`。Full 是全系列共用模板，实际主板／屏幕由有线工具的 TOML 配置选择；原厂身份与校准必须由保护式烧录流程从当前设备保留，不能将模板占位值当成设备备份。

默认 `Makefile` 构建允许明确标记的未签名 App 的 Recovery；带签名的 App 仍须通过验签。强制验签入口是：

```powershell
make -f Makefile.signed SDK_ROOT=D:/InkTagSDK
```

`Makefile.signed` 固定选择 required，不能通过环境或普通命令行变量改为 optional。不传签名 App 时仅输出 raw 和原生证明，明确提示未生成 OTA／Full。要一键产出强制验签的 Full，传入已有的有效签名 App：

```powershell
make -f Makefile.signed full SDK_ROOT=D:/InkTagSDK SIGNED_APP=D:/Firmware/app-signed.img
```

脚本使用内置公钥验签，并要求签名 App 的代码、版本和 epoch 与本次编译一致；不匹配就停止，不生成替代包。`full` 目标缺少签名 App 会在编译前报错。签名核验依赖 `cryptography`（可在自己的 Python 环境中安装）。两个公开入口均不签包、不访问密钥；官方发布继续使用 required 策略及私有签发流程。App OTA 不会改变设备已有 Recovery 的策略。

也可在仓库根目录直接使用 PowerShell：`pwsh -NoProfile -File ./build.ps1 -SdkRoot D:/InkTagSDK`（默认 optional，自动打包），或加 `-OtaSignaturePolicy required -SignedApp D:/Firmware/app-signed.img -RequireFull`。`make` 的非默认 PowerShell 路径通过 `POWERSHELL` 指定；非默认 Keil 路径可在 PowerShell 入口指定。

默认构建已自动生成 OTA。仅需重新包装一份明确选定的 raw App 时，也可单独执行：

```powershell
python tools/build/package_ota.py --input '<build-id>/InkTag-da14585_aesl.bin' --output app-unsigned.img --version 0.2.1.1
```

路径请替换为实际 App raw 文件。上传需使用能读取 Recovery 策略的配套 WebUI／CLI；旧版及未知策略设备仍要求官方签名，现有设备须先安装对应 Recovery 才能接收未签名包。格式及 SHA-256 校验规则见 [DA OTA](../protocol/da-ota.md)。此命令不制作 Full、不更换设备 Recovery。

Full 打包与实际有线写入是两个步骤。构建不会访问设备；现有要求签名的有线工具不会因生成了未签名 Full 自动改变准入策略。请核对配套工具支持范围，见[烧录说明](flashing.md)。

```powershell
python tools/build/run_host_tests.py --sanitize
```

此命令运行完整 C 检查，在 `.local/host-tests/` 保存逐项编译／运行结果。它使用内存后端，不连接 BLE、调试器或设备。

公开源码同时包含有界时间／电压测试的实现及回归。另装 `pytest` 后，可运行 `python -m pytest tests/test_da_bounded_tests.py -q`；测试直接编译生产处理函数、调度和完成事件，不需要 SDK 或私有构建产物。协议及原生日期范围见[测试说明](../protocol/da-tests.md)。

## 当前格式与固定尺寸

当前 App 原生语义版本为 0.2.1；Recovery 独立版本为 0.2.1。协议0x10、兼容epoch1、DAV1/layout-v1不变。已确认旧Recovery未提供版本时，维护CLI按1.0.0/legacy-default显示；新版本从设备字段读回。不是SDK版本、CRC或构建后缀。

| 限制 | 固定范围 |
| --- | ---: |
| 可签名 App raw | 最大95648 B，另留96 B签名尾 |
| 原生 App载荷／OTA容器 | 95744 B／95808 B（含64 B头） |
| Recovery容器 | 32768 B |
| 图槽区 | 68 KiB，不借用扩大可执行区 |

节气资源的固定来源、生成输入与重建步骤见 [节气生成说明](../tools/calendar/README.md)。正常编译直接使用已提交资源。

## 推送前检查

在 Git 检出目录运行以下检查。它核对实际索引中的路径和内容、发布清单及所有可达提交，拒绝私钥、指令文件、设备备份和构建产物；强制加入索引也不能绕过检查。正常公钥和固件验签代码保留。

```powershell
python tools/build/check_publish.py --history
```
