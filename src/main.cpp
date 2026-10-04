// main.cpp — CALQL8 firmware for Teensy 4.1 (Teensyduino / Arduino framework).
//
// SKELETON: the timing-critical logic lives in the platform-independent
// headers (clock_engine.h, sequencer.h) and is fully unit-tested on the
// host via `make test`. This file wires that engine to the hardware.
// Sections marked TODO are the remaining bring-up work.
//
// Build: see docs/BUILD.md (PlatformIO recommended).

#include <Arduino.h>
#include "clock_engine.h"
#include "sequencer.h"

// ============================================================================
// Pin map — TODO: finalize against the enclosure layout.
// All jack I/O lives on the flat rear shelf, top-facing (see docs/DESIGN.md).
// ============================================================================
namespace Pins {
    // Clock / reset inputs. These see 3.3V logic AFTER the input conditioning
    // stage (comparator / divider) described in docs/DESIGN.md.
    constexpr int CLOCK_IN = 2;   // TODO: use a GPT/QuadTimer input-capture pin for best precision
    constexpr int RESET_IN = 3;   // TODO

    // 8 trigger outputs (3.3V GPIO -> op-amp buffers -> ~5V jack outputs).
    constexpr int TRIG_OUT[8] = {4, 5, 6, 7, 8, 9, 10, 11};  // TODO

    // 3x3 button matrix (8 channel keys + center shift key).
    constexpr int ROW_PINS[3] = {12, 13, 14};  // TODO
    constexpr int COL_PINS[3] = {15, 16, 17};  // TODO

    // 8 probability pots (one per channel).
    constexpr int PROB_POT[8] = {A0, A1, A2, A3, A4, A5, A6, A7};  // TODO

    // OLED + encoder for the retro-LCD-style UI.
    constexpr int ENC_A = 18;      // TODO
    constexpr int ENC_B = 19;      // TODO
    constexpr int ENC_BTN = 20;    // TODO
    // OLED: TODO — SSD1306/SH1106 over SPI or I2C; pick pins here.

    // Key LEDs live under the clear MX keycaps.
    // TODO: choose drive scheme (shift register like 74HC595, or addressable
    // LEDs) and pin it here.
}

// ----------------------------------------------------------------------------
ClockEngine clock_;
Sequencer   seq;

// --- clock input capture -----------------------------------------------------
// The ISR does the absolute minimum: timestamp the edge. The main loop feeds
// the timestamp to clock_.onClockPulse(). For the tightest timing, replace
// attachInterrupt() with a GPT/QuadTimer input-capture channel (TODO).
volatile uint32_t g_pulseMicros = 0;
volatile bool     g_pulseFlag = false;

void clockIsr() {
    g_pulseMicros = micros();
    g_pulseFlag = true;
}

// --- 64-bit microsecond clock ------------------------------------------------
// micros() is uint32_t and rolls over ~every 71 minutes. Maintain a
// monotonic 64-bit microsecond clock for the engine. TODO: implement.
static uint64_t nowUs64() {
    // TODO: track micros() rollover and accumulate into a uint64_t.
    return (uint64_t)micros();  // placeholder — replace before use
}

// --- trigger outputs ---------------------------------------------------------
// Triggers are short pulses (~8 ms). fireTrigger() raises the output;
// the main loop lowers it after TRIG_PULSE_US.
constexpr uint32_t TRIG_PULSE_US = 8000;
elapsedMicros trigTimer_[Sequencer::NUM_CHANNELS];
bool          trigActive_[Sequencer::NUM_CHANNELS] = {};

void fireTrigger(int ch) {
    digitalWrite(Pins::TRIG_OUT[ch], HIGH);
    trigTimer_[ch] = 0;
    trigActive_[ch] = true;
}

// --- UI state (TODO: flesh out) ----------------------------------------------
// Modes: perform (grid = mutes / manual triggers) vs. edit (OLED menu).
// Shift-key grammar — see docs/DESIGN.md:
//   tap channel key      -> toggle channel mute
//   shift + channel key  -> channel solo / clear
//   hold channel + turn encoder -> that channel's probability (also on pots)
//   double-tap channel   -> manual trigger
//   (momentary glitch/stutter button -> TODO, decide placement)

// ============================================================================
void setup() {
    // TODO: pinMode() for trigger outs, matrix rows/cols, encoder, reset in.
    // TODO: attachInterrupt(Pins::CLOCK_IN, clockIsr, RISING) — or input capture.
    // TODO: ADC setup for the 8 probability pots (averaging for stability).
    // TODO: OLED init + splash screen ("CALQL8").
    // TODO: LED driver init.

    // Clock source decision: slave if a clock is present, else internal master.
    // TODO: detect clock presence (e.g. pulses seen in the first 500 ms).
    clock_.setMaster(120.0);
    // clock_.setSlave(4.0);  // 4-PPQ analog clock input

    // TODO: seed from a floating analog pin / Teensy temp sensor for entropy.
    seq.setSeed(0xCA1C008u);  // placeholder
}

// Simple button-matrix scan with debounce. TODO: implement properly;
// returns a bitmask of pressed keys, edge-detected by the caller.
static uint16_t scanMatrix() {
    // TODO
    return 0;
}

void loop() {
    uint64_t now = nowUs64();

    // 1. Clock input: drain the ISR flag.
    if (g_pulseFlag) {
        g_pulseFlag = false;
        // TODO: extend the 32-bit ISR timestamp to 64 bits consistently
        // with nowUs64() before passing it in.
        clock_.onClockPulse((uint64_t)g_pulseMicros);
    }

    // 2. Advance the engine; evaluate the sequencer on every tick.
    {
        uint64_t base = clock_.tickCount();
        int n = clock_.advance(now);
        for (int i = 0; i < n; ++i) {
            seq.onTick(base + i);
            for (const auto& e : seq.events()) fireTrigger(e.channel);
        }
    }

    // 3. Trigger pulse widths: lower outputs after TRIG_PULSE_US.
    for (int ch = 0; ch < Sequencer::NUM_CHANNELS; ++ch) {
        if (trigActive_[ch] && trigTimer_[ch] >= TRIG_PULSE_US) {
            digitalWrite(Pins::TRIG_OUT[ch], LOW);
            trigActive_[ch] = false;
        }
    }

    // 4. TODO: scan the 3x3 matrix, apply the shift-key grammar
    //    (mutes, solos, manual triggers, encoder-target selection).

    // 5. TODO: read the 8 probability pots -> seq.channel(i).probability
    //    (with hysteresis so the ADC noise doesn't fight the encoder).

    // 6. TODO: encoder + OLED UI — per-channel divide/multiply, Euclidean
    //    (k/n/rotation), ratchet count, pattern length, swing, step gates,
    //    master BPM / slave status, in a retro-LCD visual style.

    // 7. TODO: reset input (rising edge) -> clock_.onReset(); seq.reset();

    // 8. TODO: USB MIDI — send MIDI clock when master; optionally follow
    //    USB MIDI clock as a second slave source; optional MIDI note output
    //    mirroring the 8 trigger channels.

    // 9. TODO: key LEDs under the clear caps — reflect trigger activity
    //    (and mute/solo state) per channel.
}
