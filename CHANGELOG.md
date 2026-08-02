# QK4 Mobile changelog

This changelog records releases of **QK4 Mobile for Android** only. QK4 Mobile
is derived from [QK4 by Mike Garcia, KF5O](https://github.com/mikeg-dal/QK4),
but upstream QK4 history is intentionally not repeated here.

## Release-note policy

- Add a new version section for every published QK4 Mobile release.
- Describe user-visible changes and important compatibility or upgrade notes.
- Use **Added**, **Changed**, **Fixed**, and **Known limitations** as needed.
- Do not list unverified radio behavior as fixed.

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
