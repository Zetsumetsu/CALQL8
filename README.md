# CALQL8

A desktop 8-channel trigger sequencer with a retro-calculator aesthetic.
Chunky MX keycaps in a 3×3 grid, eight probability pots, a retro-LCD-style
OLED UI — hands on the front, cables off the back. Teensy 4.1 brain.

## Concept

CALQL8 is a performance instrument for rhythm. Eight trigger channels live
in a single desktop enclosure styled like a vintage desk calculator: a
sloped key field up front, and a flat rear shelf with top-facing 3.5mm
jacks (clock in, reset in, 8 trigger outs) so patching is a top-down
glance — no lifting or turning the unit, and cables never cross your hands.

The 3×3 grid of mechanical switches (8 channel keys around a center shift
key, LEDs under clear caps) is the performance surface: mutes, fills, and
manual triggers played live. Eight pots ride per-channel trigger
probability. An OLED + encoder handles the deep stuff — divide/multiply,
Euclidean rhythms, ratchets, swing — rendered in a dot-matrix retro-LCD
style to match the costume.

## Key features

- **Rock-solid clock** — master/slave clock engine at 96 PPQ with hardware
  input capture, filtered period measurement, and phase re-sync on every
  incoming pulse: timing error is bounded and can never accumulate into
  drift. Freewheels at the last tempo if the external clock stops.
- **8 trigger channels** — per-channel clock divider, multiplier/ratchet,
  Euclidean pattern generator (k hits / n steps / rotation), per-step and
  per-channel probability, swing, per-step gates.
- **Performance-first UI** — 3×3 MX grid + shift-key grammar (tap = mute,
  shift + key = solo, double-tap = manual trigger), 8 probability pots,
  momentary glitch/stutter.
- **USB-C** — power plus class-compliant USB MIDI (MIDI clock in/out);
  doubles as a DAW sequencer/controller.

## Hardware

- Teensy 4.1 (600 MHz Cortex-M7)
- 9× MX mechanical switches (3×3) with LEDs under clear keycaps
- 8× potentiometers, OLED display + rotary encoder
- 3.5mm jacks: clock in, reset in, 8× trigger out (rear top-facing shelf)
- Input conditioning (clock/reset → 3.3V), op-amp buffers (3.3V → ~5V triggers)

Full design capture: [docs/DESIGN.md](docs/DESIGN.md).

## Repo layout

```
src/clock_engine.h   platform-independent master/slave clock engine
src/sequencer.h      platform-independent 8-channel trigger sequencer
src/main.cpp         Teensy 4.1 firmware skeleton (Arduino framework)
test/test_engine.cpp host unit tests (g++)
docs/DESIGN.md       design document
docs/BUILD.md        toolchain / flashing / bring-up guide
```

## Build & test

Host unit tests (no hardware needed):

```sh
make test    # expect ALL PASS
```

Teensy firmware: PlatformIO with the Teensy platform is recommended —
see [docs/BUILD.md](docs/BUILD.md). `src/main.cpp` is a skeleton with
clearly-marked TODOs; the engine it drives is fully tested on the host.

## Origins

CALQL8 is a from-scratch rebuild inspired by **EasyEi8ht**, an
8-trigger sequencer originally built on the Arduino Nano. Credit to the
original coder, **Ozerik**. CALQL8 shares no code with it — new
platform, new timing core, new design — but the spark came from there.

## License

MIT — see [LICENSE](LICENSE).
