# CALQL8 — Build Guide

Two build targets: the **host unit tests** (engine + sequencer, plain g++)
and the **Teensy 4.1 firmware** (PlatformIO or Teensyduino).

## Host unit tests (engine + sequencer)

The timing-critical code (`src/clock_engine.h`, `src/sequencer.h`) is
platform-independent C++17. Tests compile and run on any host with g++:

```sh
make test
```

Expected output ends with `ALL PASS`. The suite covers: master tick counts
at known BPM, per-channel divider ratios, ratchet sub-trigger counts and
spacing, slave mode with jittered input (no accumulated drift), flywheel
behavior, Euclidean pattern vectors, probability bounds + seeded
determinism, swing offsets, and reset behavior.

```sh
make clean   # remove build/
```

## Teensy 4.1 firmware — PlatformIO (recommended)

1. Install [VS Code](https://code.visualstudio.com/) + the
   [PlatformIO extension](https://platformio.org/).
2. Create a PlatformIO project around this repo's `src/` (or copy
   `src/` into a new PlatformIO project), with this `platformio.ini`:

```ini
[env:teensy41]
platform = teensy
board = teensy41
framework = arduino
build_flags = -Isrc
```

3. Connect the Teensy 4.1 over USB and press **Upload**. PlatformIO
   invokes the Teensy Loader automatically; press the Teensy's program
   button if the loader asks for it.

`src/main.cpp` is currently a **skeleton**: the engine is wired to the
hardware with clearly-marked TODO sections (pin map, input capture,
button-matrix scan, OLED/encoder UI, pot ADC, trigger pulse widths, reset
input, USB MIDI). Bring-up order suggestion:

1. Get the pin map finalized against the enclosure layout.
2. Verify the 8 trigger outputs with a fixed master clock (120 BPM).
3. Bring up clock-in: start with `attachInterrupt()`, move to GPT/QuadTimer
   input capture for the tightest slave timing.
4. Button matrix + shift logic, then key LEDs.
5. Pots → per-channel probability.
6. OLED + encoder UI.
7. Reset input, USB MIDI clock.

## Teensy 4.1 firmware — Teensyduino + Arduino IDE (alternative)

1. Install the Arduino IDE, then Teensyduino from
   [pjrc.com](https://www.pjrc.com/teensy/td_download.html).
2. Open `src/main.cpp` (rename to `main.ino` inside a `main/` sketch
   folder, keeping the headers alongside it), select
   **Tools → Board → Teensy 4.1**, and Upload.

## Flashing notes

- First flash on a fresh Teensy 4.1: the Teensy Loader handles it over
  USB — no bootloader juggling needed (unlike raw STM32 DFU flows).
- If the board ever seems unresponsive, press its physical program button
  to force the loader.
- The firmware skeleton compiles to a tiny binary; the engine headers add
  negligible flash/RAM on the 600 MHz Cortex-M7.

## Repository layout

```
calql8/
├── src/
│   ├── clock_engine.h   # platform-independent master/slave clock (96 PPQ)
│   ├── sequencer.h      # platform-independent 8-ch trigger sequencer
│   └── main.cpp         # Teensy 4.1 firmware skeleton (Arduino framework)
├── test/
│   └── test_engine.cpp  # host unit tests (g++)
├── docs/
│   ├── DESIGN.md        # full design capture
│   └── BUILD.md         # this file
├── Makefile             # `make test`
├── README.md
└── LICENSE (MIT)
```
