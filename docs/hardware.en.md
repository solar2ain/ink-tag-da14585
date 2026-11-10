# Hardware and features

[中文](hardware.md) | English

The DA14585 firmware targets Hema AESL-2.9 and AESL-4.2 boards. Configure the panel, orientation, timezone and display content through the shared InkTag WebUI or CLI.

## Boards

| Item | Details |
| --- | --- |
| Board models | `da14585_asel29`, `da14585_asel42` |
| MCU | DA14585, 96 KiB SRAM |
| Firmware and image storage | 256 KiB external SPI Flash, including 68 KiB for stored images |
| Clock | On-board 32.768 kHz crystal |
| Bluetooth name | App: `IKT_DA_XXXX`; Recovery: `IKT_DA_XXXX_OTA` |
| Default label | `InkTag XXXX`, separate from the Bluetooth name |

The application is loaded from SPI Flash into SRAM. Firmware, BLE state, buffers and stack share SRAM; unused Flash cannot extend executable application memory. See the [Renesas DA14585 product information](https://www.renesas.com/en/products/da14585?partno=DA14585-00000AT2) and [SDK Flash boot guide](https://lpccs-docs.renesas.com/Tutorial_SDK6/flash_prog.html).

## Panel configurations

| Panel | Controller and native resolution | Configuration |
| --- | --- | --- |
| Original AESL-2.9 E029A33N8C380 / HINK-E029A33-A0 | SSD1619, 128×296, BWR | Default in the asel29 complete image |
| High-resolution 2.13-inch Hema | SSD1619, 128×250, BWR | Select the actual resolution |
| Original AESL-4.2 | UC8176, 400×300, BWR | Default in the asel42 complete image |
| ZKC42VM / ZKV42VM 4.2-inch replacement | SSD1683, 400×300, BWR | Use SSD1683 rather than the original panel's controller |
| Huawei 3.98-inch A0 | JD79665AA, 768×552, BWRY | Four-color direct transfer; a full frame exceeds image-slot storage |

Match the controller, recipe, colors, native dimensions and viewport. Equal physical size or resolution does not establish controller compatibility. The native 128×296 panel is only 128 pixels high in landscape and uses the compact calendar layout. Layout follows usable width and height, not the “2.9-inch” name.

## Image capacity

Slot capacity follows the full frame, a 96-byte image header and 4 KiB sector alignment:

`count = floor(68 KiB / align_up(full-frame bytes + 96, 4 KiB))`

| Configuration | Frame bytes | Flash per slot | Reference count |
| --- | ---: | ---: | ---: |
| 128×250 BWR | 8,000 B | 8 KiB | 8 |
| 128×296 BWR | 9,472 B | 12 KiB | 5 |
| 400×300 BWR | 30,000 B | 32 KiB | 2 |
| 768×552 BWRY | 105,984 B | Exceeds image storage | 0 |

Use the device's reported capacity and available slots. RLE decodes to a full frame and does not increase stored-image capacity. Panels with zero slot capacity can still receive direct images, but cannot store them or run autonomous slideshows.

Slot IDs remain stable and may have gaps after deletion. Image names allow up to 35 UTF-8 bytes. Set names and types during upload or replacement; separate editing is unavailable. Replacement writes in place, so a power interruption can lose the previous image in that slot. Reorganize stored images after changing panel geometry or slot stride.

The board page reports factory-default and effective display-signal pins and allows changes to MOSI, CLK, CS, DC, RESET, BUSY and optional EN. Changes persist; restoring default pins removes only the pin override and preserves calendar and other settings. Complete-flash configuration defines the factory defaults. Flash, SWD and fixed BS signals remain protected, and EN polarity is configurable. Set all six display signals and EN to unused to disable the display; BLE configuration remains available. Select only GPIOs physically present on the package and connected to the intended signals.

This firmware omits the SSD1675-specific 1.54-inch driver. Common layouts, other panel drivers and source code remain available.

## Voltage, temperature and power

At a valid voltage of 2.6 V or lower, red-capable screens show a red battery icon. This display rule does not change Flash or OTA low-voltage protection.

Voltage readings show the supply state; they are not a calibrated battery-capacity meter. An unavailable reading is shown as unavailable, not zero.

Temperature comes from a panel controller that supports readback, not the MCU. Normal refreshes sample temperature, and the cache remains valid for 15 minutes. Connection-temperature support samples when the cache is invalid and the panel is idle. Busy, failed or unsupported readings remain unavailable.

An aging battery, long leads, poor contacts or the voltage drop across a series ammeter can cause Flash writes to fail. Storage and communication faults can cause similar errors. Read status and check power first; do not lower the write-protection threshold.

## Features

The firmware provides a calendar, clock, device page, direct image transfer, image-slot management and autonomous slideshows. The device page shows the Bluetooth name, board model, brand and built-in product QR code. Device labels can be changed through the client.

Settings reset and user-data cleanup are separate operations. Signed OTA and safe Recovery resume are supported. App-only OTA does not replace Recovery, the bootloader or fonts.

Timekeeping uses the on-board low-frequency crystal. Synchronize after initial setup, battery replacement or a power interruption.
