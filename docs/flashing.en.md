# Flashing and Recovery

[中文](flashing.md) | English

See the [distribution flashing guide](https://github.com/solar2ain/ink-tag/blob/main/docs/en/flash.md) and [DA package](https://github.com/solar2ain/ink-tag/tree/main/firmware/da14585_aesl). The current portable DA package still declares complete writing unavailable. Offline validation does not establish successful device programming.

The solar-term table is stored in GUI resources. App-only OTA does not update the dates already stored on the device. Install compatible updated GUI resources through the resource-update or complete-installation path actually supported by the delivery.

## Inputs and boards

Stock installation or a Recovery upgrade needs an appropriate Full package and wired tool. App OTA updates neither Recovery, loader nor fonts. Default source builds retain raw/native proof and create unsigned OTA/Full files under `packages/`. Old or mandatory-verification Recoveries refuse unsigned Apps; existing signature-required wired tools also refuse unsigned Full. Packaging is not hardware qualification; see [building](building.en.md).

One `da14585_aesl` family uses asel29.toml (SSD1619 128×296 BWR), asel42.toml (UC8176 400×300 BWR), or asel42-ssd1683.toml (HINK-E042A13-FPC-A0/SSD1683 400×300 BWR, recipe1). Model aliases do not establish PCB markings. Verify controller, BUSY polarity and wiring; do not import another unit's identity/calibration.

## Wiring and dependencies

Windows x64 and externally obtained J-Link7.88j/SWD. Retain the complete package backend/runtime/DLL/helper/configuration layout. The source repository does not bundle these programmer tools, SDKs or private keys.

Connect the board's verified supply, GND, SWDIO and SWCLK; never5V. Confirmed asel29 usesTP5=SWCLK andTP6=SWDIO. **Do not connect VPP/TP3.** Check physical labels and supplier documentation for the exact board. Select an exact JLinkSerial with multiple probes. Avoid probing during clock/power observations; unknown write/readback/identity/power results stop without replay.

## Delivered tools versus source paths

Use actual package files and an explicitly supplied signed Full; the following paths are examples, not a pinned old build:

```powershell
$DaTools = 'D:/Downloads/InkTag/firmware/da14585_aesl/tools'
& "$DaTools/setup.ps1"
& "$DaTools/check.ps1"
& "$DaTools/validate.ps1" -Chip DA14585 -Firmware 'D:/Firmware/InkTag-da14585_aesl-full.bin' -Config "$DaTools/configs/asel29.toml"
& "$DaTools/flash.ps1" -Chip DA14585 -Firmware 'D:/Firmware/InkTag-da14585_aesl-full.bin' -Config "$DaTools/configs/asel29.toml" -DryRun
```

Only use entries actually supplied by the package; where validate.ps1 is not exposed, follow its README's flash.ps1 DryRun. Maintained development tools live under `firmware/ports/dialog/da14585/tools/`; this public source snapshot excludes that complete backend and its controlled SRAM helper, and does not provide root `tools/flash.ps1`. The built Full is a firmware template, not a standalone programmer. Copying one script cannot supply its backend and dependencies.

Validation/DryRun check the package/signature/CRC/layout/configuration without target access, current-unit capture or writer qualification. The portable writer gate remains closed. Removing DryRun or addingYes/RetryDetect does not enable it. No bypass is provided.

## Full scope and success

DA Full clears settings/labels/time/slideshow/pin overrides and the entire68KiB image store, reinstalls packaged fonts/defaults and initializes both Recovery journals without preserving the previous epoch. OTA retains antirollback.

Protect current-unit factory identity, BLE address/type, calibration and OTP. Operate only on the firmware's first256KiB; larger physical Flash beyond that remains untouched and unverified, not securely erased. Clear/read back the supported32-byte RAM reset trace before the sole final start; unknown diagnostic layouts stop. Confirmation defaults to cancel; Yes never bypasses safety checks.

Success requires every mandatory write/readback/identity check and the sole final startup. Recovery/loader CRC is not publisher authentication. A100% phase, offline validation or zero HEALTH fields are not proof of completed flashing.

## Recovery version and return

Use the maintained shared CLI containing the read-only channel:

```console
inktag-cli device -n IKT_DA_1234 --no-time-sync recovery enter
inktag-cli device -n IKT_DA_1234_OTA --no-time-sync recovery status
inktag-cli device -n IKT_DA_1234_OTA recovery resume-app
```

Status performs no update, resume or clock write. Confirmed old unversioned Recovery, including exact SDK string `v_6.0.24.1464`, displays1.0.0/legacy-default. Versioned Recovery reports the actual DIS version/source=device. Timeout, short reads and unknown formats remain errors. Older distributed CLI versions may lack status; check help, never infer installed software from CRCs or build suffixes.

resume-app/exit share a90-second default App return window. After exact App re-attestation and reconnect, the maintained client synchronizes time unless --no-time-sync was requested. Explicit valid reconnect-wait values remain. Source version declarations are separate from actual installed firmware.
