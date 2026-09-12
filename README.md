# QK4 Android

QK4 Android is a phone-focused Android client for Elecraft K4 transceivers. It preserves the proven radio-control, TCP/TLS, panadapter-stream, and TX/RX audio architecture of QK4 while replacing its desktop-oriented interaction model with a landscape touch interface.

The application is under active development and is intended for testing with an Elecraft K4/K4D on the same network. Version 1.0.5 is the current ARM64 test build.

![QK4 Mobile v0.8.0 console](https://github.com/worldwidedx/QK4-Android/releases/download/v0.8.0/QK4-Mobile-v0.8.0-Screenshot.png)

## Project lineage

QK4 Android is a derivative of [QK4](https://github.com/mikeg-dal/QK4), created by Mike Garcia, KF5O. Android development and phone UX adaptation are by [worldwideDX.com](https://worldwidedx.com/).

This repository retains the GNU General Public License v3 used by the upstream project. See [LICENSE](LICENSE).

## Current capabilities

The shared ADIF logbook supports manual Add/Edit, import/export, and contacts
from FT8/FT4 and SSTV. SSTV Log QSO reviews RX CALL or TO CALL before saving.
Long-press DXLIST → Log opens the same logbook from the radio console.
Logbook Setup supports automatic QRZ uploads using a securely stored logbook
API key, plus manual sending and locked upload-status checkboxes. Standard
QRZ ADIF upload fields are preserved in exports. See [QRZ setup](docs/QRZ_LOGBOOK.md).

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
- Integrated SSTV transmit and receive with 22 modes, image composition,
  templates, automatic reception, callsign identification, and RX history
- Touch-scrollable DX prefix reference with natural alphanumeric sorting and prefix/country search
- Android landscape layout and touch-safe scrolling
- Local non-decaying Peak Hold and local WTR CLRS waterfall brightness control
- Release-signed APK distribution support

See [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) for the verified
state and next work. Contributors changing screen rotation or device-class
layouts must also follow the [screen orientation policy](docs/ORIENTATION_POLICY.md).

The `codex/ft8-ft4-portrait` branch adds an initial portrait FT8/FT4 receiver
and practice workspace, reached by tapping the former SSTV Fn button (hold
for SSTV). Its 3 kHz view uses QK4's main waterfall renderer: tap for RX,
hold to set TX, pinch or use +/− to zoom. Station colors retain their WSJT-X
meanings. Hide waterfall to expand the station list; Rows offers Comfortable,
Compact, and Dense text layouts for busy bands. Existing CTR2-MIDI support is
included: one assigned button switches between the RX and TX audio tuning targets;
tapping a main-frequency digit explicitly selects RF tuning at that step.
Assignable **FT8/FT4: Switch RX/TX tone** and **FT8/FT4: Set tone frequency**
actions provide one workflow: short press chooses the
tone, the dial moves a dashed preview, and long press sets that frequency.
Setting TX
enables Hold TX; setting RX focuses My QSO near that frequency while All keeps
the full band. See [CTR2 controls](docs/CTR2_UI_ACTIONS.md) and the
[rollback marker](docs/FT8_CTR2_ROLLBACK.md).
The current dial target remains visible with the waterfall hidden.
The compact QSO panel hides unused exchange rows, and smaller activity-filter
buttons leave more vertical space for received stations.
A compact RF power slider sits directly below the frequency and follows the
K4 power setting. Both modules now offer remembered TEST-mode audio calibration,
with shared automatic drive reduction and transmit-stop protection. SSTV
requires a matching calibration. See [digital transmit levels](docs/DIGITAL_TX_LEVEL.md)
for the implemented behavior and pending K4 hardware acceptance.
Live FT8/FT4 transmission is now available through Call/CQ, using the saved
calibrated drive and timed program audio. Halt TX cancels queued and active
transmissions. This device-test build supports standard exchanges in DATA-A
with split and TEST off. Signal reports are measured automatically using
WSJT-X's FT8/FT4 methods and the 2500 Hz reference bandwidth. See
[docs/FT8_FT4_SCOPE.md](docs/FT8_FT4_SCOPE.md) for the implemented scope,
logging format, and the remaining RF validation work.

## Supported target

| Item | Current development target |
|---|---|
| Platform | Android 8.0 (API 26) or later |
| ABI | ARM64 (`arm64-v8a`) |
| Android package | `com.w9wdx.qk4phone` |
| UI | Landscape touch UI; the compact phone layout is temporarily used on all display sizes, including tablets |
| Framework | Qt 6.11.1 |
| Android API | Minimum 26, target 34 |
| Radio | Elecraft K4/K4D |

Other platforms remain present in the inherited QK4 source, but this repository's supported product target is Android. Physical acceptance testing has been performed on a Samsung Galaxy S26 Ultra; test other phone families before treating them as validated.

## Recommended K4 operating settings

These settings are practical starting points for remote operation and SSTV.
Band conditions, interference, antenna performance, and individual
installations may require different settings.

### SSTV receive

- Use **AGC-F** as the normal starting point. If rapid gain changes or pumping
  appear to degrade reception, compare results with **AGC-S**.
- Enable **K4 RX Auto Attenuation**. This allows the K4 to reduce analog
  front-end gain automatically when exceptionally strong signals threaten
  receiver dynamic range. It complements AGC-F; AGC and RF gain operate later
  and cannot correct front-end overload.
- Leave the preamp off unless it produces a genuine weak-signal improvement.
  On noisy HF bands, extra preamp gain often raises both signal and noise
  without improving decoding.
- Use a receive passband wide enough to preserve the complete SSTV tone range,
  approximately **1200-2300 Hz**, with reasonable margin on both sides.
- Avoid filter shift settings that cut off the lower synchronization tones or
  upper image tones.
- Start with **NB, NR, SSNR, manual notch, and APF off**. Add processing only
  when it improves actual image decoding.
- Use **NB** for repetitive impulse noise and select the lowest effective
  level. Aggressive blanking can distort SSTV tones or create artifacts when
  strong signals are nearby.
- Use **NR or SSNR selectively** for difficult signals. Compare reception with
  processing on and off; a signal that sounds cleaner to the ear does not
  necessarily decode better.
- Rear-panel analog **LINE OUT** levels do not control the network audio stream
  used by QK4 Mobile.

### SSTV transmit

- Prefer the K4's **DATA** mode for SSTV transmission. It provides a clean
  audio-data path without speech compression.
- **USB** may also be used when compression is set to zero and TX EQ is flat.
- Do not use speech processing, aggressive transmit EQ, or other voice
  enhancement on SSTV tones.
- Use only the RF power needed for reliable communication and account for the
  high duty cycle of SSTV transmissions.
- Confirm transmission quality with an independent receiver, WebSDR recording,
  or another SSTV decoder when initially configuring the station.

See the
[Elecraft K4 Operating Manual](https://ftp.elecraft.com/K4/Manuals%20Downloads/K4%20Built-In%20Operating%20Manual%20rev%20D6/K4BuiltInOperatingManualrevD6.html)
for detailed descriptions of AGC, attenuation, preamplifiers, noise blanking,
noise reduction, filtering, and DATA-mode operation.

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
test-windows.cmd -Action Test
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

Development phones that still contain the former `com.ai5qk.qk4phone` debug
package can migrate its private QK4 settings and SSTV data once before the old
package is removed. Follow the guarded procedure in
[docs/BUILD_ANDROID_WINDOWS.md](docs/BUILD_ANDROID_WINDOWS.md); the migration
utility verifies every copied file before it permits removal.

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
scripts/                  Guarded development and migration utilities
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
