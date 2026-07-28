# Build and test reference

## Windows build

Use the repository script:

```powershell
.\build-android.cmd -Action Configure
.\build-android.cmd -Action Build
.\build-android.cmd -Action Apk
```

Read `docs/BUILD_ANDROID_WINDOWS.md` when dependency discovery fails.

## Device installation

Install only when requested:

```powershell
.\build-android.cmd -Action Install
```

Use `adb devices` first when more than one device may be attached.

## Validation levels

1. Source inspection: confirm signal/slot wiring, state ownership, and command selection.
2. Build: compile the Android ARM64 target and package an APK.
3. Visual test: inspect the landscape phone UI for clipping, unreachable controls, and gesture conflicts.
4. Radio test: verify commands, RX audio, TX audio, and resulting K4 state on actual hardware.

Do not promote a result from one level as proof of the next.

