# CALQL8 — Bill of Materials (prototype)

Prototype build on perfboard ("project circuit board"). Powered by a 12V DC
wall adapter — see Power & protection. Check off what you already have;
anything marked **buy** is worth ordering.

## Core

| Qty | Part | Spec / note |
|---|---|---|
| 1 | Teensy 4.1 | The brain. You have this. |
| 2 | 24-pin female headers | Socket the Teensy so it's removable (2×24 = 48 pads). **Buy** if you don't socket boards. |

## Switches, keycaps, LEDs

| Qty | Part | Spec / note |
|---|---|---|
| 9 | MX-style mechanical switches | 8 channel keys + center shift. Linear or tactile — your call. |
| 9 | Clear/transparent 1u keycaps | LEDs must shine through. |
| 9 | 3mm LEDs | Color is your aesthetic call (red for the 70s-calculator vibe). Get a few spares. |
| 2 | 74HCT595 shift registers | LED drivers — 3 Teensy pins drive all 9 LEDs. **HCT**, not HC: HCT inputs accept the Teensy's 3.3V while the chip runs at 5V. Chain the two for 16 outputs. |
| 9 | 220Ω resistors, 1/4W | LED current limiting (≈14 mA at 5V for red LEDs; use 330Ω if you want them dimmer). |
| 9 | 1N4148 diodes | Key-matrix anti-ghosting, one per switch. You *will* press 3+ keys at once playing mutes live — without these the matrix ghosts. |

The 9 switches wire as a 3×3 matrix (6 pins). Internal pullups — no extra resistors.

## Pots & encoder

| Qty | Part | Spec / note |
|---|---|---|
| 8 | 10kΩ linear pots (B10K) | Per-channel probability. Panel-mount to suit your faceplate. |
| 8 | Knobs | Your pick — calculator aesthetic says chunky. |
| 1 | EC11 rotary encoder with push switch | Menu navigation + click. Teensy internal pullups handle the wiring. |

## Display

| Qty | Part | Spec / note |
|---|---|---|
| 1 | OLED module, SSD1306 0.96" SPI (or 1.3" SH1106) | SPI version preferred — much faster refresh than I2C for the retro-LCD UI. Most modules run on 5V or 3.3V; check yours. |

## Trigger outputs (8×) + clock out

Two-stage 2N7000 MOSFET drivers per channel — non-inverting, so outputs
idle at 0V and fire positive-going pulses. (The old 74HCT245 plan can't do
the jumper-selectable 5V/8V trigger rail, so it's out.)

| Qty | Part | Spec / note |
|---|---|---|
| 18 | 2N7000 N-channel MOSFETs | 2 per channel × 9 channels (8 triggers + clock out). Buy 20 — they're pennies. |
| 9 | 1kΩ resistors, 1/4W | Pull-ups to the V_TRIG rail (drain of 2nd stage). |
| 9 | 10kΩ resistors, 1/4W | Pull-ups to 3.3V (drain of 1st stage). |
| 9 | 100kΩ resistors, 1/4W | Gate pulldowns on the 1st stage — keeps outputs at a safe 0V while the Teensy boots. |
| 9 | 100Ω resistors, 1/4W | Series protection on each output jack. |
| 1 | 3-pin male header + jumper shunt | V_TRIG select: 5V / 8V for the trigger driver rail. **Buy** the shunt if you don't have one. |
| 1 | LM7808 | 8V rail for the hot-trigger option (from the 12V input). |
| 1 | SPDT toggle switch (optional) | Alternative to the jumper if you want 5V/8V switchable from outside. Your call. |

Per channel: Teensy GPIO → 100kΩ pulldown → Q1 gate; Q1 drain → 10kΩ to 3.3V → Q2 gate; Q2 drain → 1kΩ to V_TRIG (jumper-selected 5V/8V) → 100Ω → jack. Firmware drives the pulse active-high (~8 ms); the two stages un-invert it back to a positive-going trigger.

*Design-doc note: DESIGN.md §7 has the full power/rail scheme.*

## MIDI out (TRS)

| Qty | Part | Spec / note |
|---|---|---|
| 2 | 220Ω resistors, 1/4W | Classic 5V MIDI output circuit (TX → 220Ω → tip, 5V → 220Ω → ring). |

TRS-A wiring per MMA RP-054 (tip = signal, ring = +V, sleeve = ground). Driven by Teensy Serial1 TX at 31250 baud.

## Clock / reset inputs (2×)

| Qty | Part | Spec / note |
|---|---|---|
| 2 | BAT54S dual Schottky diodes | Input clamps to 3.3V — protects the Teensy from hot Eurorack signals (±10V happens). |
| 4 | 10kΩ resistors, 1/4W | Divider pairs... |
| 2 | 20kΩ resistors, 1/4W | ...for a ~3:1 divider on each input, plus series protection. Values from your kit are fine — exact ratio isn't critical with the clamp diodes behind it. |

## Jacks

| Qty | Part | Spec / note |
|---|---|---|
| 14 | 3.5mm panel-mount jacks | 8 trigger outs + clock in + reset in + clock out + clock thru + MIDI out + 1 spare. The MIDI jack must be **TRS (stereo)** type; the rest mono. Thonkiconn-style or whatever's in the parts bin. Top-facing on the rear shelf per the design. |

## Power & protection

| Qty | Part | Spec / note |
|---|---|---|
| 1 | 2.1mm DC barrel jack, panel-mount | 12V DC in, center-positive (the standard wall-wart). **Buy** if you don't have one. |
| 1 | Buck converter, 12V→5V, ≥1.5A | Off-the-shelf LM2596 module is fine for the prototype — feeds the 5V rail (Teensy VIN, LEDs, MIDI circuit). A linear 7805 would work but burns ~2W+ as heat; the buck runs cool. |
| 1 | 1N5819 Schottky diode | Reverse-polarity protection on the 12V input (series). |
| 1 | Polyfuse, ~750mA hold | Overcurrent protection on the 12V input. Resets itself — no fuses to replace. |
| 1 | 12V DC wall adapter, 2.1mm center-positive, ≥1A | The mains power. You may already have one. |

Power chain: 12V in → polyfuse → Schottky → buck → 5V rail → Teensy VIN (its onboard regulator makes 3.3V); 12V also feeds the 7808 → 8V rail for the trigger jumper option.

## Passives & board

| Qty | Part | Spec / note |
|---|---|---|
| 10 | 100nF ceramic capacitors | Decoupling, one per IC plus spares. Non-negotiable — sprinkle liberally. |
| 3 | 10µF electrolytic capacitors | Bulk decoupling on the 5V and 3.3V rails. |
| 1–2 | Large perfboard (~100×160 mm) | The "project circuit board". Two if you want the jack shelf on its own board. |
| — | Hookup wire, solder, standoffs | Assumed on hand. |

*Programming access: the Teensy 4.1 keeps its micro-USB port for firmware upload — mount it so the port stays reachable through a panel cutout or at the enclosure edge. No USB-C breakout needed on the panel anymore.*

## Resistor summary (for kit checking)

100Ω ×9 · 220Ω ×11 · 1kΩ ×9 · 10kΩ ×13 · 20kΩ ×2 · 100kΩ ×9 — plus a general E12 assortment kit covers everything else.

## Not in this BOM

- **Enclosure / faceplate** — you're designing and building it (sloped key field + rear jack shelf per DESIGN.md §2).
- **Key feel decisions** — switch type, keycap profile, slope angle: mock up in cardboard first (§9).
- **SD card** — only needed if preset slots get built later (future idea).

*Last updated 2026-10-04. Prototype BOM — 12V wall power, TRS MIDI out, 5V/8V jumper-selectable triggers.*
