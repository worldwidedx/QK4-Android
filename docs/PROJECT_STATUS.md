# Project status

Last updated: 2026-07-27

## Last verified Android behavior

The application connects to the K4 and supports RX audio, TX microphone audio, radio control, touch panadapter tuning, direct frequency entry, tuning-step synchronization, CW text decode, and the phone-oriented control panels.

The most recent device testing confirmed:

- CW text decode activates and receives decoded text.
- Selecting a frequency digit updates the VFO tuning rate.
- Panadapter touch tuning respects the selected tuning rate.
- Right-side control-panel dragging no longer triggers unintended buttons.
- Radio RX and streamed audio are operating.

## Next session: first priority

### 1. WTR CLRS must be local

The Elecraft controls distinguish:

- `#WBS`: waterfall color range, documented CAT values 5-30.
- `#WFC`: waterfall color palette/mode.

The K4 presents WTR CLRS as approximately 0.5-3.0 in 0.1 increments. The Android control should open an adjustment field above the DISP buttons, in the same location used by the NB adjustment. It needs a current value plus `-` and `+`.

For this application, WTR CLRS is a local waterfall-rendering setting. It should change the mapping between incoming dB/bin intensity and the existing local waterfall color lookup table. It should not send a radio command, and it must not be implemented as palette selection.

The exact K4 internal transfer curve is not public. Match the documented behavior: increasing the value produces brighter colors for the same signal intensity.

### 2. Peak Hold must be local

The K4 command `#PKM` only reports or changes Peak Mode on the radio display. Neither the public streaming description nor QK4's decoded PAN packet contains a radio-generated peak trace.

Implement a local trace using the incoming spectrum bins:

```text
peak[i] = max(peak[i], current[i])
```

Requirements:

- Draw the peak trace as a red line above the live spectrum.
- Maintain independent peak arrays for panadapter A and B.
- Reset the relevant array when Peak is enabled, the connection changes, or center/span/bin geometry changes.
- Keep the local display toggle authoritative for the local trace.
- Sending `#PKM` to keep the physical radio display synchronized is optional and must not be treated as the source of peak data.
- Start with persistent maxima. Measure the physical K4 before adding decay behavior.

## Deferred

- DR+ display support remains deferred until its correct source and state semantics are confirmed.

## References

- [Elecraft K4 manuals](https://elecraft.com/pages/k4-high-performance-direct-sampling-sdr-manuals)
- [Elecraft K4 Programmer's Reference](https://ftp.elecraft.com/K4/Manuals%20Downloads/K4ProgrammersReferencerev.D12.html)
- [Upstream QK4](https://github.com/mikeg-dal/QK4)

