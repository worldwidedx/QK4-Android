# QK4 Android

QK4 Android is a phone-focused Android client for Elecraft K4 transceivers. It preserves the proven radio-control, TCP/TLS, panadapter-stream, and TX/RX audio architecture of QK4 while replacing its desktop-oriented interaction model with a landscape touch interface.

The application is under active development and is intended for testing with an Elecraft K4/K4D on the same network. Version 0.8.1 is the current ARM64 release.

![QK4 Mobile v0.8.0 console](https://github.com/worldwidedx/QK4-Android/releases/download/v0.8.0/QK4-Mobile-v0.8.0-Screenshot.png)

## Project lineage

QK4 Android is a derivative of [QK4](https://github.com/mikeg-dal/QK4), created by Mike Garcia, KF5O. Android development and phone UX adaptation are by [worldwideDX.com](https://worldwidedx.com/).

This repository retains the GNU General Public License v3 used by the upstream project. See [LICENSE](LICENSE).

## Current capabilities

QK4 Mobile supports every known operator-facing capability that the K4 exposes
for remote operation through its documented command, control, display, and
streaming interfaces. Functions that Elecraft has not implemented or exposed
to remote clients, such as BAND/MEM, remain outside the application's control.

- K4 profile management and TCP/TLS connection
- RX audio streaming for the main and sub receivers
- Microphone audio and PTT transmission
- Remote CW keying from Bluetooth LE and USB MIDI paddle interfaces, including
  TinyMIDI and HaliKey MIDI presets plus learnable custom MIDI mappings
- K4-synchronized paddle orientation, Iambic mode, keying weight, CW speed,
  local sidetone, and paddle testing
- USB-C headset RX/TX hot-swap, plus Bluetooth/USB mixed-route support where Android provides it
- Android hearing-aid RX routing when the operating system exposes a dedicated hearing-aid output
- VFO A/B display, tuning, direct frequency entry, and selectable tuning steps
- GEN shortwave-listening band bank with persistent per-band frequency recall
- Touch tuning from the panadapter
- Spectrum and waterfall display, including mini-pan
- Mode-aware Main RX, Sub RX, TX, radio-control, display, function, and message controls
- FM repeater shift/offset, PL tone, and programmable DTMF controls
- RIT/XIT jog control
- CW text decoding
- F1-F8 macro editing and execution
- Android landscape layout and touch-safe scrolling
- Local non-decaying Peak Hold and local WTR CLRS waterfall brightness control
- Release-signed APK distribution support

See [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) for the verified state and next work.

## Supported target

| Item | Current development target |
|---|---|
| Platform | Android 8.0 (API 26) or later |
| ABI | ARM64 (`arm64-v8a`) |
| UI | Landscape touch UI; the compact phone layout is temporarily used on all display sizes, including tablets |
| Framework | Qt 6.11.1 |
| Android API | Minimum 26, target 34 |
| Radio | Elecraft K4/K4D |

Other platforms remain present in the inherited QK4 source, but this repository's supported product target is Android. Physical acceptance testing has been performed on a Samsung Galaxy S26 Ultra; test other phone families before treating them as validated.

## Build on Windows

Install:

- Qt 6.11.1 with the Android ARM64 kit and a matching Windows desktop host kit
- Android SDK, platform tools, and NDK
- Android Studio's bundled Java runtime or another compatible JDK
- CMake and Ninja, normally installed by the Qt Maintenance Tool

The ARM64 Opus headers and static library used by the current Android build are kept under `third_party/android/opus` so the repository does not depend on the original development PC's directory layout.

From PowerShell or Command Prompt:

```powershell
build-android.cmd -Action Doctor
build-android.cmd -Action Configure
build-android.cmd -Action Apk
```

To make a distribution APK, use the external release keystore and the
temporary signing environment variables documented in
[docs/BUILD_ANDROID_WINDOWS.md](docs/BUILD_ANDROID_WINDOWS.md):

```powershell
build-android.cmd -Action Apk -DeploymentType Release
```

To install on a connected phone with USB debugging enabled:

```powershell
build-android.cmd -Action Install
```

The script discovers normal Qt and Android SDK locations. Any nonstandard location can be supplied through these environment variables:

| Variable | Purpose |
|---|---|
| `QK4_QT_ANDROID` | Qt Android ARM64 kit directory |
| `QK4_QT_HOST` | Matching Qt Windows host kit directory |
| `ANDROID_SDK_ROOT` | Android SDK directory |
| `ANDROID_NDK_ROOT` | Android NDK directory |
| `QK4_JAVA_HOME` | Preferred JDK directory for this build |
| `JAVA_HOME` | Fallback JDK directory |
| `QK4_CMAKE` | Full path to `cmake.exe` |
| `QK4_NINJA` | Full path to `ninja.exe` |
| `QK4_OPUS_ROOT` | Alternate Android Opus installation |

Detailed setup and troubleshooting are in [docs/BUILD_ANDROID_WINDOWS.md](docs/BUILD_ANDROID_WINDOWS.md).
For a transfer checklist, including what is intentionally *not* stored in Git, see [docs/PORTABILITY.md](docs/PORTABILITY.md).

## Source layout

```text
android/                  Android manifest, Gradle configuration, and icons
src/audio/                Opus and Qt audio engine
src/controllers/          UI and radio orchestration
src/dsp/                  Spectrum, panadapter, and waterfall rendering
src/models/               K4 state and CAT response handling
src/network/              TCP/TLS and K4 streaming protocol
src/settings/             Local application settings
src/ui/                   Shared and Android-adapted widgets
third_party/android/opus/ ARM64 Android Opus development files
.codex/skills/            Repository-local Codex development skill
```

## Security and local data

Radio profiles and passwords are runtime data and are not stored in this repository. Do not commit profile exports, logs containing credentials, keystores, signing passwords, APKs, build trees, or phone screen captures.

Production distribution requires a private Android signing key. Keep signing credentials outside the repository and provide them only through the supported build environment.

## Development guidance

Read [AGENTS.md](AGENTS.md) before making changes. The central rule is to preserve QK4's known-good connection, audio, and radio-control methods. Android work should adapt presentation and input behavior without inventing alternate radio plumbing.
