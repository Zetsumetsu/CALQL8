# CALQL8 — Design Document

A desktop 8-channel trigger sequencer with a retro-calculator aesthetic,
built around a Teensy 4.1. This is the complete design capture: form factor,
control surface, clock engine, sequencer features, electronics, enclosure,
rejected ideas, and open questions.

## 1. Vision

CALQL8 is a performance instrument for rhythm: eight trigger channels in a
box that looks like it belongs on a desk next to a ledger, not in a rack.
The 3×3 key grid *is* a numpad — the design leans into the calculator
metaphor instead of fighting it. Chunky beveled keycaps with a satisfying
clack, a sloped body, and an OLED UI rendered in a retro-LCD style
(dot-matrix font, faux 7-segment BPM readout).

It is **desktop-only, single enclosure**. There is no Eurorack module
version and no breakout box: everything lives in the one box, hands on the
front, cables off the back.

## 2. Form factor

Two-tier profile, like a proper desk calculator:

- **Front: sloped key field** carrying the 3×3 performance grid, the 8
  probability pots, and the OLED + encoder. Slope angle TBD (see §9):
  calculator-steep (~10°) keeps it typable; drum-machine-steep (~25–30°)
  reads better standing up. Mock up in cardboard before committing.
- **Rear: flat horizontal shelf** with **top-facing 3.5mm jacks** — clock
  in, reset in, clock out, 8 trigger outs, clock thru, and MIDI out (TRS).
  Jacks point straight up so patching is a top-down glance; no lifting or
  turning the unit to see where things plug in. Cables rise and drape
  behind, never crossing the player's hands.

The shelf must be deep enough that a row of plugged right-angle cables
clears comfortably. Jack labels read from the player's seated position
(text oriented toward the front edge, not the back).

The user designs the faceplate and builds the enclosure.

## 3. Control surface

### 3.1 The 3×3 grid (the soul of the instrument)

Nine MX mechanical switches in a 3×3 grid: 8 channel keys surrounding a
center **shift** key. LEDs live under clear keycaps and show trigger
activity per channel (and mute/solo state). The grid is the performance
surface — mutes and fills played live with the fingers, cables nowhere
near.

### 3.2 Probability pots

Eight potentiometers, one per channel, hardwired to per-channel trigger
probability. Probability is the most playable parameter on a trigger
sequencer — riding eight probabilities live while the grid handles mutes
is the core performance gesture of the instrument.

### 3.3 OLED + encoder

Replaces the old play/config toggle entirely. All deep per-channel
parameters live here with real labels instead of toggle-switch Morse code:
divide/multiply, pattern length, Euclidean (k/n/rotation), ratchet count,
swing, per-step gates, master BPM / slave status. Rendered in retro-LCD
style to match the calculator costume.

## 4. Shift-key interaction grammar

The center shift key is the most important button on the panel — the "="
key of the instrument:

| Gesture | Action |
|---|---|
| Tap channel key | Toggle channel mute |
| Shift + channel key | Channel solo (or clear, TBD in UI testing) |
| Hold channel + turn encoder | Edit that channel's probability (pots do this too) |
| Double-tap channel key | Manual trigger |
| Momentary glitch/stutter control | Dedicated button (decided 2026-10-04 — see §4.5) |
| Shift + encoder click | Toggle **take capture** (see §4.5) |
| Shift + encoder hold (> 0.6 s) | Clear take buffer |

## 4.5 Takes — capture, perform, release

The take system records a performance *version* of the pattern and lets the
player move between the composed original and the performed take.

- **What a take is:** a per-channel snapshot of live state — mute/solo
  state, the 8 probability pot positions, and per-step gate edits as they
  stand at capture time. Momentary gestures (glitch fills, manual triggers)
  are *not* captured; they stay live-only.
- **Capture toggle:** Shift + encoder click. The OLED status line shows
  `TAKE` while a take exists. Capturing again overwrites the previous take.
- **Morph dial:** the encoder doubles as the morph dial whenever a take
  exists (decided 2026-10-04 — no new hardware, no panel change). At one
  end the composed pattern plays; at the other the take plays; in between,
  each step is probability-blended — the pattern drifts between the two
  versions rather than hard-switching. The OLED shows a take-morph readout
  while active.
- **Clear:** Shift + encoder hold clears the take buffer (dial snaps back
  to original).
- **Ephemeral by design (decided 2026-10-04):** takes live in RAM only —
  capture, perform, release. Power off wipes the take; a new session means
  programming a new sequence. No EEPROM persistence.

## 5. Clock engine

The timing core (`src/clock_engine.h`) is platform-independent C++ and
fully unit-tested on the host. Key properties:

- **96-PPQ internal timebase.** One shared tick stream; all channels derive
  from it, so outputs cannot drift relative to each other.
- **Master mode:** ticks scheduled from a fixed integer-microsecond period
  derived from BPM. No floating-point phase accumulation, no drift.
- **Slave mode:** external pulses are timestamped (input-capture ISR in
  firmware; the ISR only records the timestamp, the main loop does the
  math) and the pulse period is tracked with an **exponential moving
  average**, so input jitter is smoothed, not passed through.
- **Phase re-sync on every pulse:** the downbeat tick fires on the pulse
  (unless the freewheeling grid already fired it within half a subdiv, in
  which case the grid carries on). Timing error is bounded by a fraction
  of one subdivided tick and **can never accumulate**.
- **Count snap:** on each pulse the tick count snaps to the nearest pulse
  boundary, deleting any extra (or restoring any missing) freewheeled
  subdiv ticks. This is what guarantees zero long-term drift of musical
  position, not just phase.
- **Multiplied subdivisions** between pulses come from the filtered period
  and are corrected at the next pulse.
- **Flywheel:** if pulses stop, the engine freewheels at the last measured
  tempo instead of stopping dead.
- **Glitch rejection:** pulse intervals outside 0.25×–4× of the filtered
  period are ignored by the measurement (but still re-sync phase), so
  double-triggers and dropouts can't corrupt the tempo estimate.
- External rates: 4 PPQ analog clock by default; 24 PPQ (MIDI-style) supported.

Verified by host tests: 500 jittered (±2 ms) pulses at 120 BPM stay within
±15 ticks of ideal over 12,000 ticks — i.e., no measurable drift.

## 6. Sequencer features

The sequencer core (`src/sequencer.h`) is also platform-independent and
unit-tested. Per channel:

- **Clock divider** — trigger every Nth 16th step (1–16).
- **Clock multiplier / ratchet** — M evenly-spaced triggers per played
  step (1–8); ratchets are scheduled on the sub-tick grid so they stay
  glued to the clock.
- **Probability** — per-channel (the 8 pots) × per-step multiplier
  (0–1). One RNG roll per step: the whole step (all ratchet hits) lives
  or dies together. Seeded RNG for deterministic behavior/tests.
- **Euclidean rhythms** — k hits over n steps with rotation, per channel.
  Selectable instead of the programmed step gates.
- **Step gates** — per-step on/off pattern (16 steps, per-channel length).
- **Swing** — 0–1, delays odd 16ths by up to a 32nd note (12 ticks).
- **Momentary glitch/stutter** — hold to override the pattern with a
  fill/stutter, release to return (UI placement TBD).
- **Reset input** — restarts pattern phase on all channels; measured slave
  tempo is kept.

## 7. I/O and electronics

- **Brain:** Teensy 4.1 (600 MHz Cortex-M7). Chosen over the original
  Arduino Nano (16 MHz ATmega328P — the root of the old slave-timing
  unreliability) for its hardware timers, input capture, GPIO, USB MIDI,
  and the large Arduino-community sequencer codebase to learn from.
- **Logic levels:** the Teensy is 3.3V; Eurorack triggers are ~5V.
- **Power:** 12V DC in via a 2.1 mm barrel jack (center-positive), or a
  rechargeable battery pack — anything in the 9–15V DC range (a 3S LiPo
  pack with protection, or a 12V lithium pack, builder's choice). A
  polyfuse and a Schottky reverse-polarity diode guard the input; then a
  buck converter (12V→5V, ≥1.5A) feeds the 5V rail, and the Teensy's
  onboard regulator makes 3.3V from the 5V rail (Teensy VIN). A 7808
  derives an 8V rail from the 12V input for the hot-trigger option —
  note it needs ≥10.5V in, so on a nearly-flat battery use the 5V
  trigger setting.
- **Trigger outputs (8×):** two-stage 2N7000 MOSFET drivers per channel —
  non-inverting, so outputs idle at 0V and fire positive-going pulses.
  The driver pull-up rail (**V_TRIG**) is jumper-selectable between **5V
  and 8V** via an internal 3-pin header (a panel toggle switch is a
  drop-in alternative if you want it on the outside). Default 5V: the
  standard Eurorack level. 8V: for gear that wants a hotter trigger.
  Firmware generates ~8ms pulses (see `TRIG_PULSE_US` in main.cpp), with
  100Ω series resistors on each output.
- **Clock out:** same MOSFET driver topology, one channel, pulled to 5V.
  Outputs the instrument's internal clock — the master tempo, or the
  slave-tracked tempo when following CLK IN — at 4 PPQ (default) or
  24 PPQ (firmware-selectable).
- **Clock thru:** buffered copy of CLK IN — zero-latency passthrough for
  daisy-chaining a clock to the next device.
- **Inputs (clock, reset):** divider + BAT54S clamp front end to 3.3V,
  with the input-capture pin on the clock input for sub-microsecond
  timestamping.
- **MIDI out:** a 3.5 mm TRS jack wired **TRS-A** per MMA RP-054
  (tip = signal, ring = +V, sleeve = ground) — the MMA-recommended
  default. Note some Korg / Make Noise gear uses TRS-B, so check the
  destination device; label the jack clearly. Classic 5V output circuit
  (2× 220Ω resistors) driven by a Teensy UART (Serial1 TX). Sends MIDI
  clock when master; optional MIDI-note mirroring of the 8 trigger
  channels. No MIDI in — external sync arrives via CLK IN.
- **Programming access:** the Teensy's micro-USB port is retained for
  firmware upload — mount the Teensy so the port stays reachable through
  a panel cutout or at the enclosure edge. USB is gone from the panel
  I/O, not from the build process.

## 8. Enclosure notes

- User-designed faceplate, user-built enclosure. Sloped console, two-tier
  (sloped key field + flat rear jack shelf).
- MX switch pitch (~19 mm) sets the grid footprint; lay out on real
  dimensions before cutting.
- Decide glitch-control placement during layout (dedicated momentary
  button vs. shift-combo).
- Ventilation is trivial (no hot parts); focus effort on key feel, label
  legibility, and cable clearance on the shelf.

## 9. Rejected ideas (with reasons)

- **Wireless/BLE trigger transmission to a rack receiver:** rejected.
  BLE connection intervals (7.5–30 ms) and millisecond-scale jitter are
  fundamentally incompatible with sub-millisecond trigger timing. The
  jitter is non-deterministic (interference, retries), so it can't be
  calibrated out — it would rebuild the exact unreliability the Teensy
  port is meant to fix. BLE remains acceptable for *non-timing* data
  (presets, config, firmware updates) if ever wanted.
- **Multicore snake (HDMI/DB-25) to a small rack breakout:** dropped in
  favor of single-enclosure simplicity. All I/O is on the unit itself;
  nothing else to build, no rack HP consumed at all.
- **Jacks on the back panel:** superseded by the top-facing rear shelf
  (§2) — patching stays visible from the top view.
- **USB-C panel connector (power + USB MIDI):** superseded 2026-10-04 by
  12V/battery power and TRS MIDI out. The panel keeps 13 jacks and no
  USB; USB survives only as the Teensy programming port.

## 10. Open questions

- Enclosure slope angle (mock up ~10° vs ~25–30° in cardboard).
- Glitch/stutter: ~~dedicated momentary button or shift-combo?~~ → **dedicated button** (decided 2026-10-04; capture toggle takes the shift-combo instead).
- Take morph dial: ~~which physical control?~~ → **encoder doubles as morph dial whenever a take exists** (decided 2026-10-04; no new hardware, no panel change).
- Shift + channel: solo vs. clear (decide in UI testing).
- Aesthetic direction: Braun-style cream minimalism vs. 70s dark red-glow
  LED-calculator — drives keycap colors, labeling, LED colors.
- Clock-thru jack: ~~include or not?~~ → **include** (decided 2026-10-04;
  zero-latency passthrough, while CLK OUT carries the musical clock).
- MIDI TRS-A vs TRS-B wiring (default A per MMA RP-054; label the jack).
- Battery pack choice (3S LiPo with protection vs. 12V lithium pack).
- Buck converter: off-the-shelf module vs. discrete buck circuit.
- Trigger level default: 5V (jumper to 8V internally).
- GitHub: public vs. private repo (repo must be created manually —
  the push integration cannot create repos).
- Attribution: original coder's name + original repo URL for the
  Origins section in README.md.
- OLED size/choice (SSD1306 vs. SH1106, SPI vs. I2C) and encoder model.
