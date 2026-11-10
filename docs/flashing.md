# 烧录与 Recovery 说明

中文 | [English](flashing.en.md)

参照 [InkTag发布库的完整烧录说明](https://github.com/solar2ain/ink-tag/blob/main/docs/zh-CN/flash.md) 和 [DA目录](https://github.com/solar2ain/ink-tag/tree/main/firmware/da14585_aesl)，以实际下载包的 README、manifest 和工具门禁为准。发布库当前DA便携工具仍声明完整写入未开放；文件验证成功不表示已经完成设备写入。

本版节气表位于 GUI 资源中。仅进行 App OTA 不会更新设备上的节气日期；要使用新版节气表，需要按实际发行支持的资源更新／完整安装路径更新兼容的 GUI 资源。

## 先选正确的文件

- 首次从原厂安装或更新 Recovery，需要匹配的完整 Full 包和有线工具；App OTA 不更新 Recovery、loader 或字体资源。
- 日常 App 升级用共享CLI／WebUI及匹配的官方签名 OTA 包。
- 源码默认构建同时生成 raw/native proof 和 `packages/` 下的未签名 OTA／Full。旧版或强制验签 Recovery 不接受未签名 App；现有强制签名的有线工具同样不会接受未签名 Full。生成文件不代表已完成设备测试，详见[编译说明](building.md)。

三个配置共用固件家族 `da14585_aesl`。asel29、asel42是已有型号别名，不是新增发布目标，也不是已确认的主板丝印：

| 配置 | 实际屏幕 |
| --- | --- |
| asel29.toml | SSD1619 128×296 BWR 原屏 |
| asel42.toml | UC8176 400×300 BWR 原屏 |
| asel42-ssd1683.toml | HINK-E042A13-FPC-A0／SSD1683 400×300 BWR，recipe1 |

相同尺寸不代表相同控制器、BUSY极性或引脚。更换屏幕后核对准确配置，不拿旧板的身份／校准备份套新板。

## 接线与依赖

DA使用J-Link／SWD，Windows x64和单独取得的SEGGER J-Link7.88j。按工具包setup/check准备便携运行依赖；软件、DLL、SRAM helper和配置必须保留完整相对布局。源码构建不提供烧录工具包、SDK或私钥。

接主供电、GND、SWDIO、SWCLK；供电电压按实际主板核对，不能接5V。已确认asel29是TP5=SWCLK、TP6=SWDIO；**不连接VPP/TP3**，不凭其它板照片猜供电点。主板接线应依据实物标记及供应商资料。无实际接线／电气证据的板先停止。

多探针须选精确JLinkSerial；走时／功耗观察中不探测。RetryDetect只等待就绪，不重试已执行的擦写或未知结果；供电、身份、容量或读回错误停止并保留证据。

## 工具包与源码路径不同

以下例子使用实际下载后的平台工具目录。把工具根和外部签名Full文件替换为真实路径，不固定某个旧build：

```powershell
$DaTools = 'D:/Downloads/InkTag/firmware/da14585_aesl/tools'
& "$DaTools/setup.ps1"
& "$DaTools/check.ps1"
& "$DaTools/validate.ps1" -Chip DA14585 -Firmware 'D:/Firmware/InkTag-da14585_aesl-full.bin' -Config "$DaTools/configs/asel29.toml"
& "$DaTools/flash.ps1" -Chip DA14585 -Firmware 'D:/Firmware/InkTag-da14585_aesl-full.bin' -Config "$DaTools/configs/asel29.toml" -DryRun
```

只有实际工具包提供的入口才可用；某个发行层未提供validate.ps1时，按其README使用flash.ps1的DryRun。维护开发环境的有线工具位于 `firmware/ports/dialog/da14585/tools/`；本公开源码快照不包含该完整后端和受控 SRAM helper，也不提供根目录 `tools/flash.ps1`。构建出的 Full 是固件模板，不是独立烧录工具；单独复制一个脚本不能补齐后端及其依赖。

validate／DryRun仅验证包、签名、CRC、布局和板型配置，不访问目标，不证明writerReady、当前电压或出厂数据保留。当前portable gate保持关闭，去掉DryRun、加Yes或RetryDetect也不能解锁；本说明不提供绕过。真正开放写入必须先完成该工具包的独立后端资格与正常发布。

## 完整写入的后果与成功标准

适用于当前DA约定：清空设置、备注、时间、轮播、引脚覆盖和全部68KiB图槽；装回包内字体／默认设置，重新初始化两页Recovery状态。不保留或比较旧Recovery epoch；OTA仍保留正常防回退。

保护当前设备的原厂身份、BLE地址／类型、校准及OTP。处理固件使用的前256KiB；更大物理Flash的额外区域保留、未回读，不能宣称整芯片安全擦除。支持的32字节RAM复位线索在唯一最终启动前清理并回读，未知诊断布局停止。Full确认默认取消；Yes仅代替确认，不能跳过安全门。

成功必须包含全部要求的写入、独立完整回读、身份核对和唯一最终启动。CRC是完整性校验，不是Recovery或loader的发布者认证。单阶段100%、offline validate、HEALTH零值均不是完整成功证明。

## Recovery版本读回与返回

配合含当前只读通道的维护共享CLI：

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync recovery enter
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

status只读，不触发恢复、升级或校时。已确认旧Recovery无软件版本或准确原SDK字符串 `v_6.0.24.1464` 时显示1.0.0/legacy-default；有明确软件版本的Recovery读真实DIS值、source=device。连接失败、短读、未知格式不当旧版。老发行CLI未必含status，先看 `recovery --help`，不能借版本字符串猜已安装的镜像。

resume-app与exit的默认App返回等待统一90秒；显式有效reconnect-wait保持。确认准确App返回并重连后，维护客户端默认校时，`--no-time-sync`保留不校时语义。源码版本不表示设备已安装；安装前核对实际设备与发行包。
