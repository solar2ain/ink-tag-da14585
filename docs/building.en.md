# Building from source

[中文](building.md) | English

Build the DA14585 App, Recovery, loader and GUI resources from this source tree.

## Dependencies

Windows, GNU Make, PowerShell 7 (`pwsh`) and Python 3.12; a legitimately licensed Keil uVision / Arm Compiler 6.24; externally obtained Renesas/Dialog SDK 6.0.24.1464. Default builds need only Python's standard library; verifying a supplied signed App additionally needs `cryptography`. Offline C checks need GCC or WSL GCC and optionally ASan/UBSan. Acquire the SDK, compiler and programmers separately under their supplier terms. SDK inventory records verify file integrity.

## Build steps

Run in the snapshot root, replacing paths with your verified external SDK. Initialize inventory only for a known lawful source; never overwrite or automatically bless changed/unknown inputs.

```powershell
python tools/sdk/da_sdk_workspace.py preserve-base --source 'D:/SDK/DA145xx_SDK/6.0.24.1464' --sdk-root 'D:/InkTagSDK'
python tools/sdk/da_sdk_workspace.py verify --sdk-root 'D:/InkTagSDK'
make SDK_ROOT=D:/InkTagSDK
```

For nondefault Keil locations add `-KeilRoot 'C:/Keil_v5'`; the default is `%LOCALAPPDATA%/Keil_v5`. Retain the external SDK and da6-base.json. The two profiles use separate `.local/da-sdk6-optional` and `.local/da-sdk6-required` object caches and build incrementally with `-b`.

`.local/builds/optional/<build-id>/` or `.local/builds/required/<build-id>/` retains raw App, Recovery, loader, GUI and native proof. Default `make` also creates a `packages/` directory in that build:

| File | Purpose |
| --- | --- |
| `InkTag-da14585_aesl-ota.img` | Explicitly unsigned App OTA with SHA-256 integrity |
| `InkTag-da14585_aesl-recovery.img` | Native container for the compiled Recovery |
| `InkTag-da14585_aesl-full.bin` | 256 KiB Full template with paired App, Recovery, loader, GUI and initialized state |
| `package-manifest.json` | Policy, component sizes/hashes, native checks and input-proof binding |

`make full SDK_ROOT=D:/InkTagSDK` is also available. Each build has its own immutable output directory. Use Full for wired installation and the App `.img` for OTA, rather than raw files. This is one family-wide template; the wired tool's TOML selects the board/panel. Its guarded workflow must preserve the actual unit's factory identity/calibration. Placeholder template bytes are not a device backup.

The default `Makefile` builds Recovery with optional signatures. Signed Apps
still require valid publisher signatures. The mandatory-verification entry is:

```powershell
make -f Makefile.signed SDK_ROOT=D:/InkTagSDK
```

`Makefile.signed` fixes the required policy against environment and ordinary
command-line variable overrides. Without a signed App it produces raw files and
native proof only, explicitly reporting that no OTA/Full was created. For a
required-policy Full, supply an existing signed App:

```powershell
make -f Makefile.signed full SDK_ROOT=D:/InkTagSDK SIGNED_APP=D:/Firmware/app-signed.img
```

The built-in public key verifies the signature; compiled code, release and epoch
must match this build. A mismatch stops packaging without a replacement image.
The `full` target without a required signed input fails before compilation.
Install `cryptography` in your Python environment for signed verification.
Neither public entry signs packages or accesses keys. Official releases retain
the separate private signing workflow. App OTA does not replace Recovery.

The entry files [build.ps1](../build.ps1), [Makefile](../Makefile) and
[Makefile.signed](../Makefile.signed) are in the repository root.
Direct PowerShell from that directory remains available: `pwsh -NoProfile -File ./build.ps1 -SdkRoot D:/InkTagSDK`
(default optional with automatic packaging), or add
`-OtaSignaturePolicy required -SignedApp D:/Firmware/app-signed.img -RequireFull`. Set `POWERSHELL` for a
nondefault Make shell executable; use the PowerShell entry for a custom Keil path.

Default builds already produce OTA. To separately repackage an explicitly selected raw App:

```powershell
python tools/build/package_ota.py --input '<build-id>/InkTag-da14585_aesl.bin' --output app-unsigned.img --version 0.2.1.1
```

Replace the input with the actual App raw path. A matching WebUI/CLI must read
the Recovery policy first. Old or unknown Recoveries require official signatures;
existing devices need the corresponding Recovery installed before unsigned OTA.
See [DA OTA](../protocol/da-ota.md) for the format and SHA-256 rules. This command
does not create Full firmware or replace a device's Recovery.

Full packaging does not access hardware or change programmer admission. Existing
signature-required programmers still reject unsigned Full files; check the
matching tool's capabilities in [flashing](flashing.en.md).

```powershell
python tools/build/run_host_tests.py --sanitize
```

The complete C matrix uses inert backends and records compile/run results under `.local/host-tests/`. It does not access BLE/probes/devices.

The public source also contains the bounded time/voltage test implementation and regression. With pytest installed, run `python -m pytest tests/test_da_bounded_tests.py -q`; it compiles production handlers, scheduling and completion without SDKs or private build artifacts. See [test operations](../protocol/da-tests.md) for native date limits and payloads.

## Versions and limits

Native App version is0.2.1; independent Recovery is0.2.1. Protocol0x10, epoch1 and DAV1/layout-v1 remain unchanged. Confirmed old unversioned Recoveries display1.0.0/legacy-default in the maintained CLI; new versions are read from the device, not inferred from SDK strings, CRCs or build suffixes.

App raw budget95648 B reserves96 B for a normal signature; native payload limit95744 B and OTA container95808 B. Recovery container32768 B. The68 KiB image store cannot expand executable capacity.

See [solar-term generation](../tools/calendar/README.md) for pinned inputs and reproduction. Normal builds use the checked-in resource.

## Check before pushing

Run in the Git checkout to verify actual indexed paths and bytes, the source manifest and all reachable commits. Private keys, agent instructions, device backups and build outputs are refused even if force-added. Public keys and firmware verification code are retained.

```powershell
python tools/build/check_publish.py --history
```
