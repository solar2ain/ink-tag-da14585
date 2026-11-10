# Usage

[中文](usage.md) | English

Get the shared client from [InkTag](https://github.com/solar2ain/ink-tag). Select the scanned `IKT_DA_XXXX` name and confirm the board and fitted panel. Finish the current transfer/refresh and disconnect the other client from the same device before switching clients; other devices are unaffected.

Replace `IKT_DA_1234` with the actual name. Use a complete name or exact `device -a/--ble-address` when multiple devices are present. macOS commonly uses a system UUID instead of a MAC address.

## Connection, time and pages

Synchronize after initial setup, battery replacement or a power interruption. A restored Flash checkpoint cannot account for time spent without power. Use `--no-time-sync` to disable implicit client synchronization.

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

The device/image group no-sync flag disables implicit synchronization, not an explicit `time sync`. Clearing the screen does not delete stored images.

## Panel and calendar

List profiles and select the matching controller, resolution and colors. For the original E029A33 128×296 panel:

```console
inktag-cli device setting screen profiles
inktag-cli device -n IKT_DA_1234 setting screen set SSD1619_128x296_BWR_STANDARD
inktag-cli device -n IKT_DA_1234 setting screen calendar --week-start 1
inktag-cli device -n IKT_DA_1234 setting screen calendar --countdown-name Trip --countdown-date 2027-01-01
inktag-cli device -n IKT_DA_1234 setting screen calendar --clear-countdown
```

Week start is 0 for Sunday or 1 for Monday. Countdown labels allow up to four characters. Visible fields follow usable dimensions: the original 128×296 panel is only 128 pixels high in landscape and needs the compact layout, regardless of its 2.9-inch designation.

Use the WebUI or `screen set --help` for orientation, timezone and viewport settings. Changing geometry or slot stride may invalidate stored images. Download anything needed before clearing and uploading for the new panel.

## Images and slideshow

Omit `--slot` for immediate direct display. Large panels support direct transfer even when a complete frame cannot fit the image-storage area.

```console
inktag-cli image -n IKT_DA_1234 --no-time-sync upload picture.png --wire-encoding auto
inktag-cli image -n IKT_DA_1234 --no-time-sync upload picture.png --slot next --name landscape --wire-encoding auto
inktag-cli device -n IKT_DA_1234 --no-time-sync slot list
inktag-cli device -n IKT_DA_1234 --no-time-sync slot show landscape
inktag-cli device -n IKT_DA_1234 --no-time-sync slot download 0 --output picture-0.png
inktag-cli device -n IKT_DA_1234 --no-time-sync display slideshow start --interval 5m
inktag-cli device -n IKT_DA_1234 --no-time-sync display slideshow stop
```

- `--wire-encoding auto` selects RLE when supported and worthwhile; `none` sends raw frames; `packbits-rle` requires device support without a raw fallback.
- `--slot next` selects an empty slot; `--slot ID` replaces it. Use real IDs from `slot list`; holes are possible.
- `--store-only` saves without refreshing. Set names and types during upload or replacement, not through separate editing commands.
- The image-name field allows 35 UTF-8 bytes; client normalization can impose further restrictions. Examples use short ASCII names.
- Interrupted in-place replacement can lose the previous image. RLE compresses transfer only; slot capacity follows the decoded full frame.

Use two different valid pictures to observe rotation. Disconnect Bluetooth after starting autonomous playback. Wait for an active refresh before sending another request. An explicit BUSY refusal is not a queued operation. For unknown outcomes, inspect state before repeating commits, OTA or deletion.

## Labels and telemetry

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync setting device-name set "Living room"
inktag-cli device -n IKT_DA_1234 --no-time-sync info
```

The default label is `InkTag XXXX`, separate from the advertised `IKT_DA_XXXX`. Labels accept up to 24 printable ASCII bytes. Relabeling does not rename Bluetooth.

Voltage is a device sample, not a calibrated battery meter. Temperature comes from the panel controller, not the MCU. Connection-temperature support samples when the cache is invalid and the panel is idle; normal refreshes also sample temperature. Busy, failed or unsupported readings remain unavailable.

For persistence or low-power errors, check the battery, short supply leads and contacts. Re-read status after restoring reliable power. Do not lower write thresholds. Storage or communication faults can produce similar errors; the error text alone does not prove a bad battery.

## Cleanup

```console
inktag-cli device -n IKT_DA_1234 setting reset-settings
inktag-cli device -n IKT_DA_1234 setting clear-user-data
```

Settings reset preserves images; user-data cleanup preserves device settings. Both require confirmation. Factory reset is unavailable.

## OTA and Recovery

Use a matching signed DA `.img` package, not a renamed complete Flash `.bin` image:

```console
inktag-cli device ota inspect InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 --no-time-sync ota update InkTag-da14585_aesl-ota.img
inktag-cli device -n IKT_DA_1234 --no-time-sync recovery enter
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

asel29/asel42 share the da14585_aesl App/OTA family; use the actual released filename and manifest, not separate model-specific target names. Keep power stable and avoid simultaneous client connections.

Recovery advertises an `_OTA` suffix and does not time out automatically. App-only OTA does not replace Recovery, the loader or fonts. Incompatible Recovery requires a matching complete firmware installation. Safe resume is allowed only for an intact application without unfinished upgrade state; do not force a rejected jump.

Successful OTA includes reconnecting and checking the running application, not just completing transfer. Preserve logs for lost connections or unknown write outcomes instead of repeating writes automatically. See [flashing](flashing.en.md).

The maintained CLI status is read-only: confirmed old unversioned Recovery or its exact original SDK string displays1.0.0/legacy-default; new0.2.1 is read from DIS/source=device. Errors and timeouts remain failures. resume-app/exit use a90-second default return budget and synchronize time after exact App re-attestation/reconnect unless group --no-time-sync was supplied. Recovery0.2.1 source is not an installed upgrade; App OTA cannot replace it.
