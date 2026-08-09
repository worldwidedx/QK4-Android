# QK4 Mobile changelog

This changelog records releases of **QK4 Mobile for Android** only. QK4 Mobile
is derived from [QK4 by Mike Garcia, KF5O](https://github.com/mikeg-dal/QK4),
but upstream QK4 history is intentionally not repeated here.

## Release-note policy

- Add a new version section for every published QK4 Mobile release.
- Describe user-visible changes and important compatibility or upgrade notes.
- Use **Added**, **Changed**, **Fixed**, and **Known limitations** as needed.
- Do not list unverified radio behavior as fixed.

## [0.7.6] - 2026-08-08

### Fixed

- TLS connections to K4 radios now bundle the required Android OpenSSL 3
  runtime and select the application-packaged libraries reliably.
- Bluetooth audio routing now retains headset RX audio through TX/RX changes
  and selects the active two-way Bluetooth endpoint for transmit audio.

### Known limitations

- Hot-swapping a USB-C audio headset after connecting to the radio is not yet
  supported. Connect the headset before connecting to the K4. Bluetooth audio
  routing is unaffected.

## [0.7.5.1] - 2026-08-04

### Fixed

- Restored the original A/B VFO mode-control geometry by removing the extra
  v0.7.5 touch padding. This keeps the SUB/DIV indicators clear of the VFO B
  frequency and meter area.

## [0.7.5] - 2026-08-03

### Fixed

- Android spectrum-renderer compatibility: use a universally supported RGBA8
  normalized spectrum texture on Android, while preserving QK4's R32F desktop
  renderer unchanged. This targets devices that render waterfall data but omit
  the normal spectrum trace.

### Improved touch operation

- Tap the green on-screen **B SET** indicator to turn B SET off quickly. B
  SET remains enabled only from CTRL to avoid accidental activation.
- Tap either displayed filter shape to cycle that receiver's setting like the
  K4 console: **FIL1 → FIL3 → FIL2 → FIL1**. The visible FIL label updates
  immediately; no redundant feedback popup is shown.
- Expanded the A and B VFO/mode touch targets without extending either one
  toward the central TX selector. Tapping the colored VFO block, its mode
  label, or a small outer/bottom margin opens that receiver's MODE popup.
- Removed the redundant “MODE controls opened” feedback flash from CTRL MODE.

## [0.7.4] - 2026-08-02

### Added

- **Experimental:** while the phone's touch-latched TX/RX control is in TX,
  an in-window shield blocks all other console touch, scroll, and button input.
  The red TX ON control remains available in the same position so the operator
  can always return immediately to RX. The shield is UI-only and does not
  alter K4 PTT, CAT, microphone, or audio-stream behavior.

### Known limitations

- The TX input shield has been physically tested during a successful contact
  on the development phone, but remains experimental pending broader field
  testing and Android-device coverage.

## [0.7.3] - 2026-08-02

### Fixed

- Restored reliable phone TX/RX operation. The touch-latched TX/RX control now
  explicitly keys the K4 with `TX;` and releases it with `RX;`, while retaining
  the upstream microphone-stream path for voice audio. This is deliberate PTT,
  not VOX.
- Corrected the Android panadapter span controls to match QK4/K4 convention:
  `+` increases span and `-` reduces span.
- Restored the VFO B cursor indicator when it is selected from the DISP menu.

### Added

- Added a local PHONE MIC slider in CTRL for phone/headset input level. It
  adjusts the pre-encode Android microphone gain and does not change the K4's
  radio MIC setting.

## [0.7.2] - 2026-08-01

### Fixed

- Reworked Android transient dialogs and menus as in-window overlays. This
  prevents the Android 17/Pixel crash caused by opening a separate Qt window
  from the live GPU-rendered radio console, while retaining the panadapter,
  TCP/TLS connection, radio control, and TX/RX audio paths.
- Closing or switching a primary phone menu now also dismisses its related
  secondary editor. In particular, confirming SSB bandwidth from TX closes the
  bandwidth editor and TX menu together, retaining the selected setting.
- Right CTRL-bank scrolling now suppresses accidental REV activation. An
  intentional tap or hold retains the original momentary REV behavior.

## [0.7.1] - 2026-08-01

### Fixed

- Removed app-side uppercase/lowercase conversion from all connection-profile
  fields. Android keyboards, including Gboard, now retain full control of
  manual Shift and case-sensitive password entry.
- Removed the redundant in-app CASE button. Name, Host/IP, Password, and TLS
  Identity fields use standard Android text input without forced case.

## [0.7.0] - 2026-08-01

First QK4 Mobile release for Android ARM64 phones.

### Added

- Landscape, touch-first K4 console optimized for phone operation.
- K4 connection-profile management, TCP/TLS radio connection, RX streaming
  audio, microphone TX audio, and PTT.
- VFO A/B controls, transmission-VFO selection, direct frequency entry,
  selectable tuning digits/steps, and step-synchronized panadapter tuning.
- Spectrum/waterfall display, mini-pan, touch panadapter tuning, and
  user-adjustable waterfall height.
- Touch panels for CTRL, TX, DISP, FN, Main RX, and Sub RX functionality.
- Main/sub AF controls; mode-aware filter BW/shift controls; RIT/XIT jog.
- CW text decode and editable/executable F1–F8 message macros.
- Local persistent red Peak Hold trace and local WTR CLRS waterfall brightness
  adjustment using Elecraft's 5–30 range.
- QK4 Mobile About screen and WorldwideDX.com attribution.
- Release-signed ARM64 APK distribution using a private PKCS#12 key.

### Fixed

- Connection-profile save/connect sequence and Android keyboard case behavior.
- Touch scrolling versus accidental button activation in control panels.
- Control-panel reachability, overlay feedback placement, macro-editor theme,
  and access to the F8 macro field.
- Compact-layout sizing, antenna/status/filter visibility, and waterfall/scope
  initial 50/50 split.

### Known limitations

- Physically validated on the Samsung Galaxy S26 Ultra only; other Android
  phone sizes and manufacturers need validation.
- The release APK can still display standard Android sideloading/Play Protect
  messaging until the app is distributed through Google Play.
- DR+ display support is deferred pending verified K4 state semantics.
