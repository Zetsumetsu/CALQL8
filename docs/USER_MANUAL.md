# CALQL8 User Manual

**8-Channel Trigger Sequencer**

*Prototype firmware — October 2026. Some secondary gestures are still being finalized during UI testing; where behavior may change, this manual says so.*

---

## 1. Meet CALQL8

CALQL8 is a desktop performance instrument for rhythm: eight channels of trigger sequencing in a box built like a vintage desk calculator. It speaks the language of drum machines and modular synths — trigger pulses out of eight jacks, a clock that can lead or follow, and a playing surface meant for fingers, not menus.

The design revolves around three performance gestures:

- **The grid** — nine chunky mechanical keys for muting, soloing, and firing channels live.
- **The probability pots** — eight knobs that thin out or thicken each channel's pattern in real time.
- **The take system** — capture a performed version of your pattern, then drift between the composed original and the take with a single dial.

## 2. Panel tour

### 2.1 Front: the sloped key field

| Control | What it does |
|---|---|
| **3×3 key grid** | Eight channel keys around a center **SHIFT** key. Clear keycaps with pink LEDs underneath show trigger activity and mute/solo state per channel. |
| **8 PROB pots** | One per channel: trigger probability, hardwired and always live. |
| **OLED display** | Retro-LCD-style screen: pattern overview, per-channel parameters, BPM, clock and take status. |
| **DATA encoder (push)** | Menu navigation and value editing. Doubles as the **morph dial** whenever a take exists. |
| **GLITCH button** | Dedicated momentary stutter/fill button. |

### 2.2 Rear: the jack shelf

Thirteen top-facing 3.5 mm jacks on the flat rear shelf, labeled to read from your seated playing position:

| Jack | Function |
|---|---|
| **CLK IN** | External clock input (analog pulses). 4 PPQ default; 24 PPQ supported. |
| **RESET IN** | Trigger input: restarts all channels to step 1. Tempo measurement is unaffected. |
| **CLK OUT** | The instrument's own clock out — your master tempo, or the slave-tracked tempo. 4 PPQ default, 24 PPQ selectable. |
| **TRIG OUT 1–8** | Per-channel trigger outputs (5V or 8V jumper-selectable, ~8 ms pulses). |
| **CLK THRU** | Buffered copy of the clock input. |
| **MIDI OUT** | MIDI clock + optional note mirroring, on a 3.5 mm TRS jack (TRS-A wiring). |

Power arrives via a 2.1 mm barrel jack (12V DC wall adapter, center-positive).

### 2.3 Trigger level: 5V or 8V

Inside the box, a jumper sets the trigger output voltage: **5V** (the standard Eurorack level, and the default) or **8V** for gear that wants a hotter trigger. It's a set-and-forget internal jumper — move the shunt, done. (A panel toggle switch can replace it if you'd rather switch from outside.)

## 3. First power-up

1. Connect the 12V DC adapter (2.1 mm barrel, center-positive). The unit powers on and boots into **master** mode at 120 BPM.
2. Patch TRIG OUT 1 to your kick drum / bass voice, TRIG OUT 2 to snare, and so on.
3. Press a channel key to hear it. Turn its PROB pot fully clockwise — the channel now fires every step.

That's the whole instrument at its simplest: keys to mute, knobs to thin.

## 4. Clock: master and slave

### 4.1 Master mode (internal clock)

Turn the DATA encoder on the main screen (or set BPM in the menu) to dial in a tempo. Patch **CLK OUT** to send that tempo to your other gear as analog clock, and CALQL8 sends MIDI clock out of the **MIDI OUT** jack at the same time.

### 4.2 Slave mode (external clock)

Patch an analog clock into **CLK IN**. CALQL8 detects the pulses and follows:

- It measures the incoming tempo with a smoothing filter, so jittery clocks don't make it jitter.
- Every incoming pulse re-syncs the downbeat — timing error can never accumulate.
- If the external clock stops, CALQL8 **freewheels** at the last measured tempo instead of stopping dead.
- Double-triggers and dropouts are rejected by the measurement, so a flaky cable won't corrupt the tempo.

You don't configure any of this. Patch the cable and play.

### 4.3 Reset

A trigger into **RESET IN** snaps all eight channels back to step 1 of their patterns. The measured tempo keeps running underneath — reset moves *where* you are in the pattern, never *how fast* it goes. This is the standard Eurorack-style reset, so it works with Pamela's Workout, BeatStep Pro, and similar sources.

## 5. Playing the grid

The 3×3 grid is the performance surface. Eight channel keys, one SHIFT key in the middle.

| Gesture | Action |
|---|---|
| Tap channel key | Mute / unmute that channel |
| **SHIFT** + channel key | Solo the channel *(prototype firmware: exact behavior being finalized)* |
| Double-tap channel key | Fire a manual trigger immediately |
| Hold channel key + turn encoder | Fine-tune that channel's probability |

LEDs under each keycap flash with the channel's triggers and show mute/solo state at a glance.

## 6. Probability pots

Each of the eight pots sets its channel's **trigger probability**: the chance that any given step actually fires. Fully clockwise, the channel plays every step; fully counter-clockwise, it goes silent; in between, the pattern breathes.

This is the most playable parameter on the instrument. Riding the eight pots live while your other hand works the grid mutes is the core CALQL8 performance gesture — patterns dissolve and re-form under your fingers without ever stopping the clock.

## 7. The OLED menus

Push the DATA encoder to enter the menus. Everything the old toggle-switch Morse code used to do now has a real label:

**Per channel:** clock divider (every Nth 16th, 1–16) · ratchet/multiplier (1–8 evenly-spaced hits per step) · Euclidean rhythm (k hits over n steps, with rotation) · 16-step gate pattern · pattern length · swing (delays off-16ths up to a 32nd).

**Master:** BPM · clock source and slave status · take status.

Turn the encoder to browse, push to select, turn to edit, push to confirm.

## 8. The take system: capture, perform, release

The take system records a *performed version* of your pattern — then lets you move between the composed original and the take, live.

**Capture.** Hold **SHIFT** and click the encoder. CALQL8 snapshots the live state of every channel: mute/solo states, all eight probability pot positions, and any per-step gate edits. The OLED status line shows `TAKE`. Capturing again overwrites the previous take. (Momentary gestures — glitch fills, manual triggers — are never captured; they stay live-only.)

**Perform.** The moment a take exists, the DATA encoder becomes the **morph dial**. At one end, the composed pattern plays; at the other, the take plays; in between, each step is probability-blended between the two — the rhythm drifts from one version to the other instead of hard-switching. The OLED shows a take-morph readout while you turn.

**Release.** Hold **SHIFT** and hold the encoder (over ~0.6 s) to clear the take. The dial snaps back to the original pattern.

Takes are **ephemeral by design**: they live in RAM only. Power off wipes the take (and the pattern); a new session means programming a new sequence. Capture, perform, release — then let it go.

## 9. Glitch / stutter

Hold the dedicated **GLITCH** button to override the pattern with a stutter fill; release to drop straight back into the groove. It never disturbs the underlying pattern or the clock — it's a momentary overlay, built for transitions and breakdowns.

## 10. MIDI out

CALQL8 speaks classic MIDI over a 3.5 mm **TRS** jack — no drivers, no computer needed:

- **Wiring:** TRS-A (tip = signal, ring = +V, sleeve = ground), the MMA-recommended standard. Most modern gear (Arturia, Novation, Elektron…) matches; some Korg / Make Noise gear uses TRS-B, so check the destination device. TRS-A-to-DIN adapter cables are widely available.
- **As master:** sends MIDI clock — your DAW or hardware sequencer follows CALQL8's tempo.
- **Note mirroring:** the eight trigger channels can optionally mirror as MIDI notes, so CALQL8 doubles as a DAW drum sequencer at no hardware cost.

There is no MIDI in — external sync arrives via the CLK IN jack.

## 11. Quick reference

| You want to… | Do this |
|---|---|
| Mute a channel | Tap its key |
| Solo a channel | SHIFT + its key |
| Fire a one-shot hit | Double-tap its key |
| Thin out a channel live | Turn its PROB pot |
| Capture a take | SHIFT + encoder click |
| Drift original ↔ take | Turn the encoder (while a take exists) |
| Clear the take | SHIFT + encoder hold |
| Stutter fill | Hold GLITCH |
| Restart all patterns | Trigger RESET IN |
| Follow an external clock | Patch it to CLK IN |
| Send clock to other gear | Patch CLK OUT to its clock in |
| Send MIDI clock | Patch MIDI OUT to its MIDI in (TRS-A) |
| Change BPM (master) | Turn encoder on main screen |

## 12. Specifications

| | |
|---|---|
| Channels | 8 trigger outputs |
| Timing resolution | 96 PPQ internal |
| Trigger outputs | 5V or 8V (internal jumper select), ~8 ms pulses, 3.5 mm |
| Clock input | 4 PPQ (default) / 24 PPQ, 3.5 mm |
| Clock output | 4 PPQ (default) / 24 PPQ selectable, 3.5 mm |
| Reset input | Trigger, restarts pattern phase |
| Clock thru | Buffered copy of clock input |
| MIDI | MIDI out only, 3.5 mm TRS-A |
| Keys | 9× Cherry MX, clear caps, pink LEDs |
| Controls | 8 probability pots, DATA encoder with push, GLITCH button |
| Display | OLED, retro-LCD-style UI |
| Brain | Teensy 4.1 |
| Power | 12V DC wall adapter (2.1 mm barrel, center-positive) |
| USB | Micro-USB (Teensy) for firmware upload only |
| Enclosure | Desktop two-tier console, user-built |

## 13. Troubleshooting

| Symptom | Check |
|---|---|
| No power | Adapter plugged in and seated? The polyfuse resets itself — wait a minute after a short and try again. |
| No triggers from an output | Channel muted? (key LED dark) · PROB pot turned fully down? · cable seated? |
| Slave tempo drifting | Clock source stable? CALQL8 re-syncs every pulse — persistent drift means the *source* is drifting. Try the flywheel: unplug CLK IN and see if it holds tempo. |
| Take won't capture | SHIFT + encoder *click* (quick press), not hold. Hold clears. |
| Encoder edits menus instead of morphing | Morph mode is only active while a take exists — check for `TAKE` on the status line. |
| OLED glitch on first boot | Power-cycle once; the display controller occasionally needs a clean re-init. |

---

*CALQL8 — capture, perform, release.*
