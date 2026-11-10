# 使用指南

中文 | [English](usage.en.md)

客户端获取方式见 [InkTag](https://github.com/solar2ain/ink-tag)。优先在 WebUI 或 CLI 中选择扫描得到的 `IKT_DA_XXXX`，确认主板和实际屏幕。切换客户端操作同一台设备前，先完成当前传输／刷新，并断开另一客户端对该设备的蓝牙连接；其他设备不受影响。

示例中的 `IKT_DA_1234` 应替换为实际蓝牙名称。`device -n/--ble-name` 接受完整名称或唯一前缀；多台设备同时存在时使用完整名称或精确 `device -a/--ble-address`。macOS 地址通常是系统 UUID，使用扫描结果即可。

## 连接、校时和页面

初次使用、更换电池或断电后先校时。固件可能恢复上次保存的时间，但不能获知断电经过多久。若需关闭客户端自动校时，在命令中使用 `--no-time-sync`。

```console
inktag-cli device scan
inktag-cli device -n IKT_DA_1234 --no-time-sync info
inktag-cli device -n IKT_DA_1234 --no-time-sync setting screen show
inktag-cli device -n IKT_DA_1234 --no-time-sync time sync
inktag-cli device -n IKT_DA_1234 --no-time-sync time show --no-time-sync
inktag-cli device -n IKT_DA_1234 --no-time-sync display calendar
inktag-cli device -n IKT_DA_1234 --no-time-sync display clock
inktag-cli device -n IKT_DA_1234 --no-time-sync display device-info
inktag-cli device -n IKT_DA_1234 --no-time-sync display clear
```

设备或图片命令组的 `--no-time-sync` 关闭隐式校时，不会阻止显式 `time sync`。清屏只改变画面，不删除图槽。

## 屏幕与日历设置

先列出预设，再选择与控制器、分辨率和颜色匹配的型号。下面用于 E029A33 的 128×296 原屏：

```console
inktag-cli device setting screen profiles
inktag-cli device -n IKT_DA_1234 setting screen set SSD1619_128x296_BWR_STANDARD
inktag-cli device -n IKT_DA_1234 setting screen calendar --week-start 1
inktag-cli device -n IKT_DA_1234 setting screen calendar --countdown-name 假期 --countdown-date 2027-01-01
inktag-cli device -n IKT_DA_1234 setting screen calendar --clear-countdown
```

每周起始日 `0` 为周日、`1` 为周一。倒计时名称最多 4 个字符；实际能显示哪些栏目取决于有效画面尺寸。128×296 原屏横屏时只有 128 像素高，采用紧凑布局，不能只因标称 2.9 英寸而套用更高的布局。

方向、时区和可视区域通过 WebUI 或 `screen set --help` 配置。修改屏幕尺寸或图槽跨度后，旧图槽可能失效；先下载需要保留的图片，按客户端提示清理，再上传匹配新屏幕的内容。

## 传图、图槽与轮播

不指定 `--slot` 时直接传图并显示；指定图槽时先保存再显示。大屏可以直接传图，即使其一张完整图片放不进图槽区。

```console
inktag-cli image -n IKT_DA_1234 --no-time-sync upload picture.png --wire-encoding auto
inktag-cli image -n IKT_DA_1234 --no-time-sync upload picture.png --slot next --name landscape --wire-encoding auto
inktag-cli device -n IKT_DA_1234 --no-time-sync slot list
inktag-cli device -n IKT_DA_1234 --no-time-sync slot show landscape
inktag-cli device -n IKT_DA_1234 --no-time-sync slot download 0 --output picture-0.png
inktag-cli device -n IKT_DA_1234 --no-time-sync display slideshow start --interval 5m
inktag-cli device -n IKT_DA_1234 --no-time-sync display slideshow stop
```

- `--wire-encoding auto` 在设备支持且有收益时选用 RLE；`none` 传输原始数据，`packbits-rle` 要求设备明确支持且不回退raw。
- `--slot next` 选择空闲图槽；`--slot ID` 替换指定位置。使用 `slot list` 返回的真实 ID，不假定编号连续。
- `--store-only` 只保存、不刷新；`--content-type image` 设置图片类型，其他类型以客户端列出的选项为准。
- 图片名称与类型在上传或替换时设置，不支持单独改名或改类型。固件的图片名称字段最多 35 个 UTF-8 字节，客户端规范化规则以帮助为准；示例采用短英文名。
- 原地替换中途断电可能丢失目标槽原图。RLE 仅压缩传输，存储容量仍按解码后的完整帧计算。

建议保存两张不同的可用图片用于轮播。启动后可以断开蓝牙，让设备自主播放。若当前刷新仍进行中，等待完成后再发新指令；明确返回 BUSY 的请求不视为已排队。操作结果未知时，先查看状态和目录，不连续重发 COMMIT、OTA 或删除命令。

## 设备备注、电压与温度

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync setting device-name set "Living room"
inktag-cli device -n IKT_DA_1234 --no-time-sync info
```

默认备注为 `InkTag XXXX`，广播名为 `IKT_DA_XXXX`。备注支持最多 24 字节可打印 ASCII 字符，用于区分设备，不改变广播名。

电压来自设备采样，不是校准电池电量表；低电量、接触电阻和刷新瞬间压降可能影响写入。温度来自屏幕控制器，不是 MCU 温度。连接测温在缓存无效且屏幕空闲时采样，正常刷新也会采样；忙碌、失败或不支持回读时显示不可用。

若出现“持久化失败”“设备存储失败”或低电量拒绝，先检查电池、短线供电和接触。更换可靠电源后重新读取状态；不要降低写入门限来掩盖问题。具体故障仍可能是存储或通信异常，不能仅凭错误文字断定电池坏了。

## 清理数据

```console
inktag-cli device -n IKT_DA_1234 setting reset-settings
inktag-cli device -n IKT_DA_1234 setting clear-user-data
```

清除设置恢复普通配置、保留图片；清除用户数据删除图片等用户内容、保留设备设置。清理操作有确认提示。本固件不提供工厂重置。

## OTA 与 Recovery

选择与主板匹配的签名应用包。DA 使用 `.img`，不要改名或用完整烧录 `.bin` 代替 OTA：

```console
inktag-cli device ota inspect InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 --no-time-sync ota update InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 --no-time-sync recovery enter
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

asel29/asel42 共用 `da14585_aesl` 的 App/OTA；以实际发布文件名和manifest为准，不存在新的asel29/asel42发布命名空间。更新期间保持稳定供电，不同时连接 WebUI 和 CLI。

Recovery 名称带 `_OTA` 后缀。App-only OTA 不更新 Recovery、启动加载器或字体资源；旧 Recovery 缺少新能力时需要匹配的完整烧录更新。Recovery 不自动超时返回应用；`resume-app` 仅在应用完整有效、没有未完成升级状态时安全返回，拒绝时不要强制跳转。

OTA 完成后客户端还需重连并核对实际运行应用，不能只把传输结束当升级成功。失联或写入结果未知时保留日志，先查当前角色与状态，不自动重复写入。首次安装和 SWD 故障处理见[烧录说明](flashing.md)。

维护CLI的status只读版本：已确认旧Recovery未提供软件版本或准确原SDK字符串时显示1.0.0/legacy-default，新0.2.1从DIS读回并标device；异常和超时仍报错。resume-app/exit默认等待90秒，确认App返回并重连后默认校时，组级--no-time-sync可关闭。源码Recovery0.2.1不表示已刷入设备，App-only OTA不替换它；
