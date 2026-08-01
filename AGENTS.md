# QK4 Android development instructions

## Start here

1. Read `README.md`.
2. Read `docs/PROJECT_STATUS.md`.
3. For Android builds, read `docs/BUILD_ANDROID_WINDOWS.md`.
4. Inspect the current implementation before changing behavior.

## Product scope

- Target an Android ARM64 phone in landscape orientation.
- Preserve the complete useful K4/K4D feature set while designing touch-first navigation.
- Keep the panadapter, VFO displays, meters, and PTT immediately accessible.
- Allow secondary controls to use touch-safe panels, popups, and scrolling rather than forcing everything onto one fixed canvas.

## Plumbing invariants

- Treat the current upstream QK4 mainline as the functional reference for K4 CAT commands, TCP/TLS connection handling, streaming protocol, RX/TX audio, and radio-state synchronization.
- Do not substitute speculative protocols or connection sequences for established QK4 behavior.
- Keep local-display features separate from radio-display CAT state when the stream does not provide rendered output.
- Do not send AF gain or other operator-setting commands merely as a connection side effect.
- PTT must represent deliberate user state. Do not emulate PTT with VOX behavior.

## UX invariants

- Design for fingers rather than mouse hover, wheel input, keyboard entry, or desktop right-click.
- For a dual-line control with a primary and alternate action, any normal tap
  anywhere on the button must invoke the primary action. The alternate action
  must use a deliberate long press; never split normal tap behavior by the
  vertical position of a finger on the button.
- Distinguish scrolling gestures from button taps.
- Provide visible feedback for controls whose resulting radio state is not visible on the main display.
- Do not place required actions outside the reachable viewport.
- Verify both single- and dual-receiver layouts.

## Build and test

- Use `build-android.cmd`; do not recreate machine-specific command lines unless diagnosing the script.
- Do not install an APK on a connected device unless the user asks.
- For radio-affecting changes, validate against the Elecraft programmer reference and the current upstream QK4 implementation.
- Never claim a radio or audio behavior is fixed without device evidence.
- Keep build directories, APKs, screen captures, UI hierarchy dumps, credentials, and signing files out of Git.

## Current priority

Implement the two local panadapter features described in `docs/PROJECT_STATUS.md`:

1. Local WTR CLRS color-range adjustment.
2. Local Peak Hold accumulation and red trace.

Do not conflate WTR CLRS (`#WBS`, color range) with WFC (`#WFC`, palette selection).
