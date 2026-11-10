# DA14585 protocol reference / 协议接口

本说明对应当前源码的App协议0x10。软件版本、协议版本、存储格式和Recovery版本是不同标识，不能相互推断。具体常量和数据结构以随源码交付的头文件及解析实现为准。

## 应用GATT / Application GATT

| 项目 | UUID |
| --- | --- |
| InkTag服务 | 62750001-d828-918d-fb46-b6c11c675aec |
| 命令／响应特征 | 62750002-d828-918d-fb46-b6c11c675aec |
| 应用版本特征 | 62750003-d828-918d-fb46-b6c11c675aec |

应用版本值为4字节：协议byte、major、minor、patch。当前声明为`10 00 02 01`，即协议0x10、软件0.2.1。UUID原生字节见[DA GATT定义](../firmware/ports/dialog/common/src/user_custs1_def.h)，软件声明见[inktag_version.h](../firmware/common/epd/inktag_version.h)。

## 帧与命令 / Framing and command IDs

请求头3字节：`opcode, transaction_id, flags`。flags固定高位标记为0x10（mask0xFC），低2位是语义位；使用[release-v1辅助函数](../firmware/common/protocol/inktag_release_v1.h)编码和校验，不能使用旧的全零flags。无语义位的HELLO、事务1请求头是`01 01 10`。

命令／状态／事件ID和相关枚举见[inktag_protocol_v2.h](../firmware/common/epd/inktag_protocol_v2.h)；长度、标志、传输描述符和保留值校验见[protocol core](../firmware/common/protocol/inktag_protocol_core.c)。设备实际回复和能力查询决定命令是否可用；保留ID不能自行改号或拿来表示另一个功能。非法长度、未知标志或无效状态按解析器拒绝，不降级接受。

屏幕profile的wire长度为17字节，版本1；引脚block为16字节。wire常量见release-v1头，具体编码／校验见[屏参和持久配置](../firmware/common/epd/inktag_epd_config.c)与[引脚配置](../firmware/common/protocol/inktag_pin_config.c)。DA原生存储结构不是直接复制主机内存结构；不要用sizeof、编译器padding或另一设备的数据猜wire格式。

## Recovery版本 / Recovery software revision

Recovery服务为标准SUOTA UUID `0000fef5-0000-1000-8000-00805f9b34fb`。原厂商SUOTA版本13及只读能力值`0d 49 4b 01 01`保持原意义；它不是Recovery语义版本。

软件版本从既有Device Information Service（0x180A）的Firmware Revision（0x2A26）读ASCII，当前源码为`0.2.1`。维护CLI先核对准确的Recovery角色／身份与SUOTA，再读取版本：

```console
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
```

已确认的旧Recovery缺该字段或准确返回原SDK字符串`v_6.0.24.1464`时，显示1.0.0且来源legacy-default；真实软件字段来源device。超时、短读、未知格式不当旧版。epoch1和DAV1/layout-v1未改；App-only OTA不升级Recovery。版本查询是只读操作，不发送返回、升级、复位或校时控制。

The supplied C definitions and parsers are authoritative. Request headers use three bytes and the0x10 frame tag, not zero flags. Firmware0.2.1, protocol0x10 and Recovery0.2.1 are separate identities. Recovery software is read through existing DIS Firmware Revision; the SUOTA protocol/capability value is unchanged. See [usage](../docs/usage.en.md) and [flashing/Recovery](../docs/flashing.en.md).
