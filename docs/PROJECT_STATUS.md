# Project status

Last updated: 2026-08-12

## Released build

**QK4 Mobile v0.7.6.6** is the current feature-release source state. It fixes
the EQ preset-name editor so it appears above the graphic-EQ popup, enlarges
the preset recall/save controls for touch use, aligns the dB and Hz labels,
and adds deliberate long-press clearing for populated presets. The shared EQ
popup applies these changes to Main RX, Sub RX, and TX.

The preceding v0.7.6.5
corrects the Main RX, Sub RX, and TX graphic-EQ **FLAT** controls: the first
tap sets flat response and the second restores the exact prior eight-band
curve. Main and Sub RX share their RX EQ restore curve, matching the K4's
shared RX EQ behavior, while TX EQ restores independently. These controls
update K4 radio EQ settings. For mobile use, start with K4 RX EQ flat and use
phone/headset tone controls for personal listening preference.

The preceding v0.7.7.0 gives the formerly unused GEN BAND control a mobile-only
shortwave-listening bank: the 14 broadcast-band labels tune the active VFO to
AM defaults and retain a persistent, local last-used frequency per GEN band.
GEN preserves the K4's normal direct-frequency and nearest amateur-band-stack
behavior; it does not alter regular BN amateur-band selection or stacking.

The preceding v0.7.6.4 fixes the four right-side CTRL long-press adjustment
editors (ATTN, NB LEVEL, NR ADJ, and NTCH MANUAL): CTRL now dismisses before
the requested popup opens, and each editor provides a visible **↩** close
control. This is a touch-layout fix only; K4 radio/audio/protocol behavior is
unchanged.

The preceding v0.7.6.3 adds an RX-only Android hearing-aid output preference
for endpoints reported as `TYPE_HEARING_AID`. The change uses the existing
native Android media playback track and device-change rebuild path; it does
not change K4 audio streaming, TX, PTT, Bluetooth headset behavior, USB-C
behavior, or microphone selection. Field validation with Starkey Livio 2400
hearing aids is pending.

The preceding v0.7.6.2 point release temporarily forces the proven compact
landscape phone layout on every Android display size, including tablets, so
unvalidated alternate tablet geometry is not selected. The original detection
logic remains commented in `src/ui/k4styles.cpp` for restoration after physical
tablet testing.

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
- Hearing-aid RX routing is unverified on physical hardware. It applies only
  where Android exposes a `TYPE_HEARING_AID` output, and TX remains on the
  phone microphone unless Android supplies a separately supported input route.
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
