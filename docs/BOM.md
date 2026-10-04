# CALQL8 — Bill of Materials (prototype)

Prototype build on perfboard ("project circuit board"). USB-powered throughout —
no external PSU needed for the prototype. Check off what you already have;
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

## Trigger outputs (8×)

| Qty | Part | Spec / note |
|---|---|---|
| 1 | 74HCT245 octal bus transceiver | 8 channels in one chip: Teensy 3.3V in → ~5V Eurorack triggers out. Tie DIR high (A→B), OE low — or to a Teensy pin if you want a master output mute. Cleaner than 8 discrete transistors and avoids the headroom problem of op-amp buffers on a 5V rail. |
| 8 | 1kΩ resistors, 1/4W | Series protection on each trigger output. |

*Design-doc note: DESIGN.md specs op-amp buffers for the final build. On USB-only 5V power an LM324 can't swing to a full 5V output, so the HCT245 is the pragmatic prototype choice. If a higher-voltage rail gets added later, revisit.*

## Clock / reset inputs (2×)

| Qty | Part | Spec / note |
|---|---|---|
| 2 | BAT54S dual Schottky diodes | Input clamps to 3.3V — protects the Teensy from hot Eurorack signals (±10V happens). |
| 4 | 10kΩ resistors, 1/4W | Divider pairs... |
| 2 | 20kΩ resistors, 1/4W | ...for a ~3:1 divider on each input, plus series protection. Values from your kit are fine — exact ratio isn't critical with the clamp diodes behind it. |

## Jacks

| Qty | Part | Spec / note |
|---|---|---|
| 12 | 3.5mm mono panel-mount jacks | 8 trigger outs + clock in + reset in + 1 clock-thru (optional) + 1 spare. Thonkiconn-style or whatever's in the parts bin. Top-facing on the rear shelf per the design. |

## Power & protection

| Qty | Part | Spec / note |
|---|---|---|
| — | (none required) | USB powers everything: Teensy's onboard regulator makes 3.3V; 5V rail comes straight from USB VBUS. Total draw ≈ 250 mA worst case (all LEDs on) — well under USB's 500 mA. |
| 1 | LM7805 (or LM1117-5.0) | **Only if** you later want barrel-jack/wall-wart power instead of USB. Not needed for the prototype. |
| 1 | 1N4007 | Reverse-polarity protection — only with the external-supply option above. |

## Passives & board

| Qty | Part | Spec / note |
|---|---|---|
| 10 | 100nF ceramic capacitors | Decoupling, one per IC plus spares. Non-negotiable — sprinkle liberally. |
| 3 | 10µF electrolytic capacitors | Bulk decoupling on the 5V and 3.3V rails. |
| 1–2 | Large perfboard (~100×160 mm) | The "project circuit board". Two if you want the jack shelf on its own board. |
| 1 | USB-C panel-mount breakout | Teensy 4.1 itself is micro-USB; this gives you the single-USB-C panel connection from the design. Optional — a panel hole for the Teensy's own connector works too. |
| — | Hookup wire, solder, standoffs | Assumed on hand. |

## Resistor summary (for kit checking)

220Ω ×9 · 1kΩ ×8 · 10kΩ ×4 · 20kΩ ×2 — plus a general E12 assortment kit covers everything else.

## Not in this BOM

- **Enclosure / faceplate** — you're designing and building it (sloped key field + rear jack shelf per DESIGN.md §2).
- **Key feel decisions** — switch type, keycap profile, slope angle: mock up in cardboard first (§9).
- **SD card** — only needed if preset slots get built later (future idea).

*Last updated 2026-10-04. Prototype BOM — final-build BOM may differ (op-amp output stage, dedicated PSU).*
