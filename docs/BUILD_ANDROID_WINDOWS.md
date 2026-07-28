# Building QK4 Android on Windows

## Required components

Install these through the Qt Maintenance Tool:

- Qt 6.11.1
- Android ARM64 kit
- Matching MinGW 64-bit desktop host kit
- CMake
- Ninja

Install Android Studio or otherwise provide:

- Android SDK platform tools
- Android API 34 platform
- Android build tools
- Android NDK 27.x
- A compatible JDK

The build script defaults to Android API 26 as the minimum runtime API and builds `arm64-v8a`.

## Automatic path discovery

`build-android.ps1` checks environment overrides first and then common locations:

- Android SDK: `ANDROID_SDK_ROOT`, `ANDROID_HOME`, then `%LOCALAPPDATA%\Android\Sdk`
- Android NDK: `ANDROID_NDK_ROOT`, then the newest NDK below the SDK
- Qt Android: `QK4_QT_ANDROID`, then the newest `C:\Qt\<version>\android_arm64_v8a`
- Qt host: `QK4_QT_HOST`, then the matching `mingw_64` kit
- Java: `QK4_JAVA_HOME`, Android Studio's `jbr`, then `JAVA_HOME`
- Opus: `QK4_OPUS_ROOT`, then `third_party\android\opus`

The script stops with a descriptive error if a required path is missing.

## Commands

Configure a fresh build tree:

```powershell
.\build-android.cmd -Action Configure
```

Compile the application library:

```powershell
.\build-android.cmd -Action Build
```

Generate a debug-signed APK:

```powershell
.\build-android.cmd -Action Apk
```

Install or upgrade on the connected device:

```powershell
.\build-android.cmd -Action Install
```

For multiple connected devices:

```powershell
.\build-android.cmd -Action Install -DeviceSerial <adb-serial>
```

The generated build tree is `build-android-arm64` and is intentionally excluded from Git.

## Moving to another PC

1. Clone or copy the repository.
2. Install the required Qt and Android components.
3. Run `build-android.cmd -Action Configure`.
4. Run `build-android.cmd -Action Apk`.
5. Enable developer options and USB debugging on the phone before using `-Action Install`.

No OneDrive path or original development-directory layout is required.

## Signing

Local APK builds use Qt's debug deployment path. Production releases require a separate Android keystore and these CMake variables:

- `QT_ANDROID_KEYSTORE_PATH`
- `QT_ANDROID_KEYSTORE_ALIAS`
- `QT_ANDROID_KEYSTORE_STORE_PASS`
- `QT_ANDROID_KEYSTORE_KEY_PASS`

Never commit the keystore or passwords.
