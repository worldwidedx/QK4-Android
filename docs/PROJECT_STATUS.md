# Project status

Last updated: 2026-08-09

## Released build

**QK4 Mobile v0.7.6.2** is the current point-release source state. It
temporarily forces the proven compact landscape phone layout on every Android
display size, including tablets, so unvalidated alternate tablet geometry is
not selected. The original detection logic remains commented in
`src/ui/k4styles.cpp` for restoration after physical tablet testing.

The preceding release-signed ARM64 build, v0.7.6.1, added USB-C headset RX/TX
hot-swap after the radio session begins while preserving the Android TLS runtime
and Bluetooth headset routing across TX/RX transitions. Where Android supports
independent routes, Bluetooth RX can remain active while a USB-C headset
microphone provides TX audio. The product package is `com.ai5qk.qk4phone`,
with Android API 26 minimum and API 34 target.

Version 0.7.4 adds an **experimental** in-window TX input shield. During a
phone-initiated transmit state, it blocks all other console touch input while
leaving the red TX ON control available to return to RX. It is UI-only and
does not alter K4 PTT, CAT, microphone, or audio-stream behavior.

Version 0.7.5 adds Android spectrum-renderer compatibility for devices that
could render waterfall data while omitting the normal spectrum trace. It also
adds touch-first B SET cancellation, receiver-specific filter cycling from the
displayed filter shapes, and more forgiving A/B MODE touch targets. The normal
spectrum renderer remains the only rendering path changed; waterfall, K4
protocol, audio, and PTT behavior are unchanged.

The only physical UI acceptance device so far is a Samsung Galaxy S26 Ultra in
landscape. Until tablet testing is available, all screen sizes deliberately use
the compact layout; broader device validation is still required.

## Verified Android behavior

- K4 profile management, TCP/TLS connection, and disconnect/error handling.
- K4 RX audio streaming plus microphone TX audio and deliberate phone PTT
  operation, physically retested on the development K4 after the v0.7.3 fix.
- USB-C headset receive and transmit hot-swap, physically tested after radio
  connection. Android reports the active USB headset microphone input during
  transmit; Bluetooth RX remains available when Android maintains a split route.
- Experimental TX input shield, physically tested during a successful contact
  on the Samsung Galaxy S26 Ultra; broader device and field testing remains
  required before treating it as fully validated.
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
  a foldable or tablet. Tablet devices temporarily use the compact phone layout;
  do not call them supported until tested.
- Android sideloading can still show a Play Protect/unknown-source notice even
  for the release-signed APK. Google Play distribution requires a signed AAB
  and Play App Signing.
- DR+ indication remains deferred until its source/state semantics are proven.
- Peak Hold is intentionally local: K4 stream data does not provide a rendered
radio peak trace. WTR CLRS is local too; do not conflate it with `#WFC`.

Version 0.7.5.1 restores the original A/B VFO mode-control geometry. The
v0.7.5 extra touch padding pushed the SUB/DIV badges toward the VFO B frequency
and meter display on some layouts; this point release removes only that padding.

## References

- [Elecraft K4 manuals](https://elecraft.com/pages/k4-high-performance-direct-sampling-sdr-manuals)
- [Elecraft K4 Programmer's Reference](https://ftp.elecraft.com/K4/Manuals%20Downloads/K4ProgrammersReferencerev.D12.html)
- [Upstream QK4](https://github.com/mikeg-dal/QK4)
