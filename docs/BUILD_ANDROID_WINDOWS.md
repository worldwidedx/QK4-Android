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

Check that all required tools and paths can be discovered without changing the build tree:

```powershell
.\build-android.cmd -Action Doctor
```

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

Generate a release-signed APK in a separate build tree:

```powershell
# Set these only for the current PowerShell session; never persist passwords.
$env:QT_ANDROID_KEYSTORE_PATH = "C:\secure-location\qk4-mobile-release.p12"
$env:QT_ANDROID_KEYSTORE_ALIAS = "qk4mobile"
$env:QT_ANDROID_KEYSTORE_STORE_PASS = "<stored securely>"
$env:QT_ANDROID_KEYSTORE_KEY_PASS = "<stored securely>"
.\build-android.cmd -Action Apk -DeploymentType Release
```

This writes the release package beneath `build-android-arm64-release`. A
release-signed APK cannot update a debug-signed installation of the same
package; remove the debug install first if you want to test the release build.

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

1. Clone the private repository (preferred) or copy a complete working tree.
2. Install the required Qt and Android components.
3. Run `build-android.cmd -Action Doctor` and resolve any missing component it reports.
4. Run `build-android.cmd -Action Configure`.
5. Run `build-android.cmd -Action Apk`.
6. Enable developer options and USB debugging on the phone before using `-Action Install`.

No OneDrive path or original development-directory layout is required.
See [PORTABILITY.md](PORTABILITY.md) for the complete transfer checklist.

## Signing

Local APK builds use Qt's debug deployment path. Production releases require a separate Android keystore and these CMake variables:

- `QT_ANDROID_KEYSTORE_PATH`
- `QT_ANDROID_KEYSTORE_ALIAS`
- `QT_ANDROID_KEYSTORE_STORE_PASS`
- `QT_ANDROID_KEYSTORE_KEY_PASS`

Never commit the keystore or passwords.

Keep the keystore and its passphrase in separate protected backups. The same
key is required for every future direct APK update. If the app is later
published with Google Play App Signing, retain this key as the Play upload key.
