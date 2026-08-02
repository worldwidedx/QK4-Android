# Project status

Last updated: 2026-08-02

## Released build

**QK4 Mobile v0.7.3** is released as a public GitHub ARM64 APK and is signed
with the permanent QK4 Mobile release certificate. It is installed and accepted
by Android on the development phone. The product package is
`com.ai5qk.qk4phone`, with Android API 26 minimum and API 34 target.

Version 0.7.3 retains the Android 17/Pixel-safe in-window dialog work and
restores explicit K4 PTT control for the phone's latched TX/RX button. A tap
sends `TX;` before microphone streaming begins; the return tap sends `RX;`.
This is phone-specific deliberate PTT behavior, not VOX.

The only physical UI acceptance device so far is a Samsung Galaxy S26 Ultra in
landscape. The compact layout is selected from logical dimensions, density, and
physical screen size; broader device validation is still required.

## Verified Android behavior

- K4 profile management, TCP/TLS connection, and disconnect/error handling.
- K4 RX audio streaming plus microphone TX audio and deliberate phone PTT
  operation, physically retested on the development K4 after the v0.7.3 fix.
- VFO A/B operation, transmission-VFO selection, tuning digit selection,
  direct frequency entry, and panadapter tuning at the selected VFO step.
- Spectrum, waterfall, mini-pan, 50/50 initial spectrum/waterfall split, and
  user-adjustable waterfall height.
- Phone-oriented Control, TX, DISP, FN, Main RX, and Sub RX touch menus;
  touch-safe scrolling and long-press alternate actions.
- TX secondary editors dismiss with their parent menu after confirmation; the
  right CTRL-bank REV control is guarded against accidental activation while
  vertically scrolling.
- AF controls for main/sub receiver; relevant slider controls and mode-aware
  filter shift/bandwidth ranges.
- RIT/XIT activation and long-press jog control.
- CW text decode screen and F1–F8 macro editor/execution.
- Local non-decaying red Peak Hold trace, reset on toggle/geometry changes.
- Local WTR CLRS 5–30 brightness adjustment; it intentionally does not send a
  CAT command because it maps the application's local waterfall LUT.

## Known boundaries / next validation

- Validate landscape usability, system insets, font scaling, touch scrolling,
  and audio behavior on smaller Android phones, a Pixel/non-Samsung phone, and
  a foldable or tablet. Do not call those devices supported until tested.
- Android sideloading can still show a Play Protect/unknown-source notice even
  for the release-signed APK. Google Play distribution requires a signed AAB
  and Play App Signing.
- DR+ indication remains deferred until its source/state semantics are proven.
- Peak Hold is intentionally local: K4 stream data does not provide a rendered
  radio peak trace. WTR CLRS is local too; do not conflate it with `#WFC`.

## References

- [Elecraft K4 manuals](https://elecraft.com/pages/k4-high-performance-direct-sampling-sdr-manuals)
- [Elecraft K4 Programmer's Reference](https://ftp.elecraft.com/K4/Manuals%20Downloads/K4ProgrammersReferencerev.D12.html)
- [Upstream QK4](https://github.com/mikeg-dal/QK4)
