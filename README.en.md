# InkTag DA14585

[中文](README.md) | English

InkTag DA14585 is Bluetooth firmware for DA14585 e-paper shelf labels. Turn a panel into a desktop calendar, clock, picture frame or information sign. Configure the display, manage pictures and update firmware using the shared InkTag WebUI or Python CLI.

E-paper reflects ambient light and retains its image after a refresh. Battery life depends on capacity, refresh frequency, Bluetooth use and hardware. Full updates commonly take several seconds or longer and may flash; cold conditions can slow refreshes or cause ghosting. Wait for the image to settle before another operation.

[Open static WebUI](https://solar2ain.github.io/ink-tag/) · [Get standalone](https://github.com/solar2ain/ink-tag/tree/main/webui-standalone) · [Release files](https://github.com/solar2ain/ink-tag/releases) · [Illustrated manual](https://solar2ain.github.io/ink-tag-manual/)

## Main features

| Feature | What you can do |
| --- | --- |
| Calendar and clock | Show dates, lunar dates and time; configure week start, countdown and orientation |
| Device page | Show the Bluetooth name, board, brand and connection QR code; set a device label |
| Panel and pins | Select a matching profile; read, save or restore display pin settings |
| Pictures | Send directly to the screen or select a saved picture by name or slot ID |
| Image management | List, download, replace and delete images; save names and types during upload |
| Slideshow | Rotate saved images at an interval, including after Bluetooth disconnects |
| Firmware updates | Install matching signed App OTA packages through WebUI/CLI and use Recovery |

## Boards and panels

Targets **AESL-2.9 and AESL-4.2** DA14585 boards, identified as `da14585_asel29` and `da14585_asel42`. Both use the `da14585_aesl` release firmware family and share App/OTA. Complete programming uses the actual board's panel and wiring configuration.

| Panel | Controller | Resolution and colors | Per slot | Approximate capacity |
| --- | --- | --- | --- | --- |
| Original AESL-2.9 E029A33 | SSD1619 | 128×296, black/white/red | 12 KiB | 5 images |
| Hema 2.13-inch | SSD1619 | 128×250, black/white/red | 8 KiB | 8 images |
| Original AESL-4.2 | UC8176 | 400×300, black/white/red | 32 KiB | 2 images |
| ZKC42VM/ZKV42VM replacement | SSD1683 | 400×300, black/white/red | 32 KiB | 2 images |

The image area totals **68 KiB**. Slots contain a complete frame, a 96-byte header and 4 KiB sector alignment. Transfer compression does not increase storage capacity; use the device's reported free slots. Preserve needed pictures before changing panel geometry, then upload images for the new dimensions.

Match controller, recipe, colors, native dimensions and visible area together. Size alone does not identify a compatible panel. Square and near-square canvases use rectangular templates. A landscape 128×296 panel has only 128 pixels of height and uses the compact calendar. See [hardware](docs/hardware.en.md) for panels, wiring and power.

## Download and installation

Get shared clients, matching firmware and programming tools from the [InkTag distribution](https://github.com/solar2ain/ink-tag). Available files are determined by the actual [Release attachments](https://github.com/solar2ain/ink-tag/releases) and manifests.

- Compatible InkTag already installed: connect through WebUI or CLI and normally update using App OTA.
- Stock device or Recovery upgrade: follow [flashing](docs/flashing.en.md), using J-Link/SWD, a matching complete image and the actual tool's available write path.
- Source compilation: [build.ps1](build.ps1), [Makefile](Makefile) and [Makefile.signed](Makefile.signed) are in the repository root. Prepare the SDK and compiler using [building](docs/building.en.md). Default `make` creates App OTA and Full under that build's `packages/` directory.

App OTA does not replace Recovery, the loader or font resources.

The solar-term table is stored in GUI resources. App-only OTA does not update the dates already stored on the device. Install compatible updated GUI resources through the resource-update or complete-installation path actually supported by the delivery.

## WebUI

Start with either distribution-provided option:

1. **Static website:** open [InkTag WebUI](https://solar2ain.github.io/ink-tag/), with no local service installation.
2. **Standalone:** download/clone the distribution and open `webui-standalone/InkTag-WebUI.html` in a compatible browser. Keep the adjacent `assets/` and other supplied resources. No website deployment is required.

Use Chrome or Edge on desktop, Chrome on Android, or a Web Bluetooth browser such as Bluefy on iPhone/iPad. Enable Bluetooth and grant browser access. Initial device selection requires a user gesture; open the page in the supported browser rather than a chat app's embedded window.

1. Click Connect and choose the scanned `IKT_DA_XXXX`; check board and firmware identity. `_OTA` denotes Recovery.
2. Select the fitted panel, colors and orientation, then synchronize time after initial setup or battery replacement.
3. Display the calendar, clock or device page, or clear the screen.
4. Import a picture, adjust cropping/orientation/colors, then display it directly or save it to a slot.
5. Manage saved images in Resources; save two compatible images before starting a slideshow.
6. For firmware updates, select a matching signed OTA package and wait for update, reconnect and verification to complete.

For an optional local service, follow the distribution's [WebUI installation guide](https://github.com/solar2ain/ink-tag/blob/main/docs/en/webui.md) and use the address printed by its startup command. Static WebUI and standalone connect to the device through the browser's Bluetooth interface.

## CLI

Install the shared client using the distribution's [CLI guide](https://github.com/solar2ain/ink-tag/blob/main/docs/en/cli.md), with Python 3.11 or newer. `inktag-cli` and `inktag` invoke the same program. Every command level supports `--help`.

Device selection follows the command group. `-n` accepts a unique name substring; use a more specific name or exact `-a` address when multiple devices match. On macOS, use the system identifier returned by the scan.

**Before another client operates the same device, finish its current transfer/refresh and disconnect that device's Bluetooth connection in the other client or browser tab.** This covers static WebUI, standalone, local WebUI and CLI. Other devices can be selected separately; closing the entire page or local service is unnecessary.

### Connection, time and pages

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

Synchronize after initial setup, battery replacement or power loss. Read time while preserving the device clock with:

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync time show --no-time-sync
```

Group-level `--no-time-sync` disables implicit synchronization; explicit `time sync` still writes time. Clearing the display does not delete images.

Source builds include bounded time-set/time-shift tests using the native calendar/clock worker, plus battery-voltage reads on supported boards. Test time remains set; explicitly synchronize afterward to restore normal time. Only runnable tests are listed. See [test operations](protocol/da-tests.md) for payloads, native date limits and offline verification.

### Pictures, slots and slideshow

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

Omitting `--slot` displays directly. `--slot next` stores in a free slot; an explicit ID replaces that image. `--store-only` saves without refreshing. Names/types are saved during upload or replacement, with names limited to 35 UTF-8 bytes. Download and replace using actual IDs from `slot list`; IDs need not be contiguous.

### OTA and Recovery

```console
inktag-cli device ota inspect InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 ota update InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

Replace the example filename with the actual release file. DA OTA uses `.img`; a complete `.bin` image cannot substitute. Recovery does not exit automatically. `resume-app` returns only to a valid complete App, waits up to the default 90-second budget, and synchronizes time after App identity verification and reconnect. `status` is read-only. Check role and state after failure rather than forcing a jump or repeating a write with an unknown outcome. See [usage](docs/usage.en.md) and [flashing](docs/flashing.en.md).

## Usage notes

- Wait for the transfer and physical refresh to finish. 100% transfer progress does not mean the screen has settled.
- Display clearing, settings reset and user-data cleanup are different operations; read the client prompt.
- Disconnect power before replacing a panel, then select its matching profile and wiring.
- Use reliable power. Check batteries and contacts after low-power or storage failures before retrying.

## Documentation

- [Usage](docs/usage.en.md): settings, pictures, slideshow, cleanup and Recovery.
- [Hardware](docs/hardware.en.md): boards, panels, capacity, pins and power.
- [Building](docs/building.en.md): dependencies and compilation.
- [Flashing](docs/flashing.en.md): initial installation, complete images and Recovery upgrades.
- [Protocol](protocol/README.md): current GATT, framing and version queries.

## Community

[QQ group 418446983 (click to join)](https://qun.qq.com/universal-share/share?ac=1&busi_data=eyJncm91cENvZGUiOiI0MTg0NDY5ODMiLCJ0b2tlbiI6ImRPVW00UnZxcWNKTU41M1JXZTQrRGpDeDRqTzI5UDkrTkdPb2U0Y1JGbE00MlcreFRuNnpkSU9QWkF3d01lTEEifQ%3D%3D&data=vETdxuw2M9SvyY0uJLAPuFrwhYCMPMCkYfiu-HzwB4-vWHpF2Jo9CtUXgTsNHzOrklunriznP8R1fytCHQWXpQ&svctype=4&tempid=h5_group_info)

## Acknowledgements

Thanks to [EPD-nRF5](https://github.com/tsl0922/EPD-nRF5) for its contributions, and to the authors and maintainers of Adafruit GFX, u8g2 and the other dependencies.

## LICENCE

InkTag uses [AGPLv3](LICENSE). Inherited third-party components retain their original licenses.
