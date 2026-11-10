# InkTag DA14585

中文 | [English](README.en.md)

InkTag DA14585 是面向 DA14585 电子价签的蓝牙固件，可以把电子纸变成桌面日历、时钟、电子相框和信息牌。使用共享的 InkTag WebUI 或 Python CLI，即可配置屏幕、管理图片和更新固件。

电子纸通过反射环境光显示，刷新后可以保持画面，适合长时间展示日期、照片和提示信息。整机续航取决于电池容量、刷新频率、蓝牙使用和具体硬件。全屏刷新通常需要数秒到十几秒，期间可能闪烁；低温下刷新可能更慢，也可能出现残影，请等待画面稳定后再操作。

[打开静态 WebUI](https://solar2ain.github.io/ink-tag/) · [获取 standalone](https://github.com/solar2ain/ink-tag/tree/main/webui-standalone) · [发行文件](https://github.com/solar2ain/ink-tag/releases) · [图文使用手册](https://solar2ain.github.io/ink-tag-manual/)

## 主要功能

| 功能 | 使用方式 |
| --- | --- |
| 日历与时钟 | 显示日期、农历和时间，设置每周起始日、倒计时与显示方向 |
| 设备信息 | 显示蓝牙名称、主板型号、品牌和连接二维码，设置设备备注 |
| 屏幕与引脚 | 选择匹配的屏参，读取、保存或恢复屏幕引脚配置 |
| 图片展示 | 直接传图显示，或保存后按图片名称、图槽 ID 选择显示 |
| 图片管理 | 查看、下载、替换或删除图片，上传时保存名称与类型 |
| 自动轮播 | 设置间隔，在已保存图片间自主切换，断开蓝牙后继续播放 |
| 固件更新 | 通过 WebUI／CLI 安装匹配的签名 App OTA 包，使用 Recovery 恢复模式 |

## 主板与屏幕

面向 **AESL-2.9 与 AESL-4.2** 主板，主控为 DA14585；主板标识分别为 `da14585_asel29`、`da14585_asel42`，发行固件家族统一为 `da14585_aesl`。两种主板共用 App／OTA，完整安装时按真实主板选取屏幕和接线配置。

| 屏幕 | 控制器 | 分辨率与颜色 | 单个图槽 | 参考图片容量 |
| --- | --- | --- | --- | --- |
| AESL-2.9 原屏 E029A33 | SSD1619 | 128×296，黑白红 | 12 KiB | 5 张 |
| 盒马 2.13 英寸屏 | SSD1619 | 128×250，黑白红 | 8 KiB | 8 张 |
| AESL-4.2 原屏 | UC8176 | 400×300，黑白红 | 32 KiB | 2 张 |
| ZKC42VM／ZKV42VM 换装屏 | SSD1683 | 400×300，黑白红 | 32 KiB | 2 张 |

图槽区共 **68 KiB**，单槽包含完整帧、96 字节图片头和 4 KiB 扇区对齐空间。传输压缩不增加存储容量，以设备报告的实际可用槽为准。更换屏幕尺寸后，先保存需要保留的图片，再按新尺寸重新上传。

选择屏幕必须同时匹配控制器、配方、颜色、原生尺寸和可视区域，不能只看英寸数或分辨率。正方形和近方形画面使用普通矩形模板。128×296 原屏横向使用时高度为 128 像素，采用紧凑日历排版。更多屏型、引脚和供电说明见[硬件说明](docs/hardware.md)。

## 获取与安装

从 [InkTag 发行库](https://github.com/solar2ain/ink-tag)获取共享客户端、与主板匹配的固件及烧录工具；可下载版本以 [Releases 实际附件](https://github.com/solar2ain/ink-tag/releases)和发行清单为准。

- 已安装兼容 InkTag 固件：通过 WebUI 或 CLI 连接，通常使用 App OTA 更新。
- 原厂设备或需要更新 Recovery：阅读[烧录说明](docs/flashing.md)，使用 J-Link／SWD、匹配的完整固件及工具。核对供电、接线和实际工具开放的写入方式。
- 从源码编译：入口在仓库根目录的 [build.ps1](build.ps1)、[Makefile](Makefile) 和 [Makefile.signed](Makefile.signed)。按 [编译说明](docs/building.md)准备 SDK 与工具链；默认 `make` 自动生成 App OTA 和 Full，产物位于该次构建的 `packages/` 目录。

App OTA 只更新应用，不替换 Recovery、启动加载器或字体资源。

本版节气表位于 GUI 资源中。仅进行 App OTA 不会更新设备上的节气日期；要使用新版节气表，需要按实际发行支持的资源更新／完整安装路径更新兼容的 GUI 资源。

## 使用 WebUI

推荐先使用发行库提供的两种方式：

1. **静态网页**：打开 [InkTag WebUI](https://solar2ain.github.io/ink-tag/)，无需安装本地服务。
2. **standalone**：下载／拉取发行库后，用兼容浏览器打开 `webui-standalone/InkTag-WebUI.html`，保留旁边的 `assets/` 等配套资源。无需部署网站。

电脑推荐 Chrome 或 Edge，Android 使用 Chrome，iPhone／iPad 可使用支持 Web Bluetooth 的 Bluefy。开启系统蓝牙，在浏览器中允许蓝牙访问；首次连接由用户在设备列表中选择并授权，不要在聊天软件的内置窗口里连接。

连接和显示步骤：

1. 点击“连接”，选择扫描得到的 `IKT_DA_XXXX`，核对主板与实际固件版本。名称带 `_OTA` 的是 Recovery。
2. 在设备显示设置中选择实际屏幕、颜色和方向，首次使用或更换电池后校时。
3. 显示日历、时钟、设备信息，或执行清屏。
4. 在图片页面导入照片，调整裁剪、方向和颜色，再选择直接显示或保存到图槽。
5. 在资源列表管理已保存图片；保存两张兼容图片后可设置间隔启用轮播。
6. 更新固件时选择匹配的签名 OTA 包，按提示等待升级、重连和版本核验完成。

如果需要本地服务，另按发行库的 [WebUI 安装说明](https://github.com/solar2ain/ink-tag/blob/main/docs/zh-CN/webui.md)安装共享客户端，并使用启动命令实际显示的访问地址。静态网页和 standalone 直接由浏览器连接蓝牙设备。

## 使用 CLI

按发行库的 [CLI 手册](https://github.com/solar2ain/ink-tag/blob/main/docs/zh-CN/cli.md)安装共享客户端，使用 Python 3.11 或更新版本。`inktag-cli` 与 `inktag` 调用同一程序，各级均可用 `--help`。

设备选择参数放在命令组之后。`-n` 接受唯一的蓝牙名称片段；多台设备匹配时使用更完整的名称或 `-a` 精确地址。macOS 使用扫描结果返回的系统标识。

**操作同一台设备时，先结束当前传输／刷新，并断开另一个客户端或浏览器标签页对该设备的蓝牙连接。** 这包括静态 WebUI、standalone、本地 WebUI 和 CLI；不同设备分别选择，不必关闭整个网页或本地服务。

### 连接、校时和页面

```console
inktag-cli device scan
inktag-cli device -n IKT_DA_1234 info
inktag-cli device -a AA:BB:CC:DD:EE:FF info
inktag-cli device -n IKT_DA_1234 setting screen show
inktag-cli device -n IKT_DA_1234 time sync
inktag-cli device -n IKT_DA_1234 display calendar
inktag-cli device -n IKT_DA_1234 display clock
inktag-cli device -n IKT_DA_1234 display device-info
inktag-cli device -n IKT_DA_1234 display clear
```

初次使用、更换电池或断电后校时。需要读取并保持设备原时间时使用：

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync time show --no-time-sync
```

组级 `--no-time-sync` 关闭隐式校时，不会阻止显式 `time sync`。清屏只改变画面，不删除图槽。

源码构建包含有界时间测试（设置时间、前后偏移并刷新日历／时钟）和可用板型的电池电压读取。测试会保留设置后的时间，结束后可显式校时恢复。固件只列出能执行的测试；协议、日期范围及离线验证见[测试说明](protocol/da-tests.md)。

### 传图、图槽和轮播

```console
inktag-cli image -n IKT_DA_1234 upload photo.jpg --wire-encoding auto
inktag-cli image -n IKT_DA_1234 upload photo.jpg --slot next --name landscape --wire-encoding auto
inktag-cli device -n IKT_DA_1234 slot list
inktag-cli device -n IKT_DA_1234 slot show landscape
inktag-cli device -n IKT_DA_1234 slot download 0 --output landscape.png
inktag-cli device -n IKT_DA_1234 display slideshow start --interval 5m
inktag-cli device -n IKT_DA_1234 display slideshow stop
inktag-cli device -n IKT_DA_1234 setting device-name set "Desk"
```

不指定 `--slot` 时直接显示；`--slot next` 保存到空闲槽，指定 ID 可替换图片。`--store-only` 只保存、不立即刷新。名称和类型在上传或替换时保存，名称最多 35 个 UTF-8 字节。下载和替换使用 `slot list` 返回的实际 ID，不假定编号连续。

### OTA 与 Recovery

```console
inktag-cli device ota inspect InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 ota update InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

示例文件名须替换为实际发行文件。DA OTA 使用 `.img`，不能用完整烧录 `.bin` 代替。Recovery 不自动超时退出；`resume-app` 在应用完整有效时安全返回，默认等待 90 秒，确认 App 返回并重连后默认校时。`status` 只读；返回失败时先核对设备角色与状态，不强制跳转或重复未知结果的写入。详见[使用指南](docs/usage.md)和[烧录说明](docs/flashing.md)。

## 使用须知

- 等待当前传输和电子纸刷新完成后再操作；传输到 100% 不代表物理刷新已结束。
- 清屏、恢复设置、清除用户数据是不同操作，执行前核对客户端提示。
- 更换屏幕前断电，重新连接后选择匹配的屏幕与接线配置。
- 保持可靠供电；低电量或存储写入失败时先检查电池和接触，不反复重试。

## 文档

- [使用指南](docs/usage.md)：配置、图片、轮播、数据清理与 Recovery。
- [硬件说明](docs/hardware.md)：主板、屏幕、容量、引脚与供电。
- [编译说明](docs/building.md)：依赖和源码编译步骤。
- [烧录说明](docs/flashing.md)：首次安装、完整固件和 Recovery 更新。
- [协议接口](protocol/README.md)：当前 GATT、帧格式与版本读取。

## 交流群

[QQ 交流群：418446983（点击加入）](https://qun.qq.com/universal-share/share?ac=1&busi_data=eyJncm91cENvZGUiOiI0MTg0NDY5ODMiLCJ0b2tlbiI6ImRPVW00UnZxcWNKTU41M1JXZTQrRGpDeDRqTzI5UDkrTkdPb2U0Y1JGbE00MlcreFRuNnpkSU9QWkF3d01lTEEifQ%3D%3D&data=vETdxuw2M9SvyY0uJLAPuFrwhYCMPMCkYfiu-HzwB4-vWHpF2Jo9CtUXgTsNHzOrklunriznP8R1fytCHQWXpQ&svctype=4&tempid=h5_group_info)

## 致谢

感谢 [EPD-nRF5](https://github.com/tsl0922/EPD-nRF5) 项目，以及 Adafruit GFX、u8g2 和其他依赖组件的作者与维护者。

## LICENCE

InkTag 采用 [AGPLv3](LICENSE) 许可。继承的第三方组件保留原许可。
