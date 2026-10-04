// clock_engine.h — CALQL8 master/slave clock engine.
//
// Platform-independent: pure C++17, no hardware dependencies. Time is kept
// in microseconds (uint64_t). The engine produces a 96-PPQ tick stream that
// the sequencer consumes.
//
// Design notes:
//  * Master mode: ticks are scheduled from a fixed period derived from BPM.
//    The schedule is kept in integer microseconds so phase never drifts.
//  * Slave mode: each external clock pulse re-anchors the tick grid.
//    - The pulse period is tracked with an exponential moving average, so
//      jitter on the input is smoothed instead of passed through.
//    - On every pulse the phase is re-synced (the downbeat tick fires on the
//      pulse, unless the freewheeling grid already fired it within half a
//      subdiv — then the grid simply continues). Timing error is therefore
//      bounded by a fraction of one subdivided tick and can never accumulate.
//    - Multiplied subdivisions between pulses come from the filtered period.
//    - If pulses stop arriving, the engine freewheels at the last measured
//      tempo (flywheel) instead of stopping dead.
//  * In firmware, onClockPulse() must be called from the main loop with a
//    timestamp captured by the input-capture ISR. Never do the filtering or
//    the resync math inside the ISR itself.

#pragma once

#include <cstdint>

class ClockEngine {
public:
    static constexpr int PPQ = 96;   // internal resolution: ticks per quarter

    enum class Mode { Master, Slave };

    ClockEngine() = default;

    // ---- configuration -------------------------------------------------
    // Internal clock at the given BPM. Re-anchors the tick grid.
    void setMaster(double bpm) {
        mode_ = Mode::Master;
        bpm_ = bpm;
        tickPeriodUs_ = usPerTickFromBpm(bpm_);
        started_ = false;
    }

    // Follow an external clock. externalPpq is the pulse rate of the input
    // (4.0 = one pulse per 16th, the usual analog clock; 24.0 = MIDI clock).
    // The flywheel is seeded at the given default BPM until pulses arrive.
    void setSlave(double externalPpq = 4.0) {
        mode_ = Mode::Slave;
        externalPpq_ = externalPpq > 0.0 ? externalPpq : 4.0;
        subdivsPerPulse_ = static_cast<int>(PPQ / externalPpq_ + 0.5);
        if (subdivsPerPulse_ < 1) subdivsPerPulse_ = 1;
        filteredPulsePeriodUs_ = 60000000.0 / (bpm_ * externalPpq_);
        tickPeriodUs_ = usPerSubdiv(filteredPulsePeriodUs_, subdivsPerPulse_);
        started_ = false;
    }

    // Live tempo tweak in master mode (keeps phase). In slave mode it only
    // updates the flywheel seed tempo.
    void setBpm(double bpm) {
        bpm_ = bpm;
        if (mode_ == Mode::Master) {
            tickPeriodUs_ = usPerTickFromBpm(bpm_);
        } else {
            // Re-seed only if we have never measured a real pulse.
            if (!havePulse_) {
                filteredPulsePeriodUs_ = 60000000.0 / (bpm_ * externalPpq_);
                tickPeriodUs_ = usPerSubdiv(filteredPulsePeriodUs_, subdivsPerPulse_);
            }
        }
    }

    // EMA smoothing for the slave period measurement, 0..1.
    // Smaller = smoother but slower to follow tempo changes.
    void setFilterAlpha(double alpha) {
        if (alpha < 0.01) alpha = 0.01;
        if (alpha > 1.0)  alpha = 1.0;
        alpha_ = alpha;
    }

    // ---- slave input ----------------------------------------------------
    // Timestamp (µs) of an incoming clock pulse. Call from the main loop
    // using the ISR-captured timestamp, in chronological order.
    void onClockPulse(uint64_t nowUs) {
        if (mode_ != Mode::Slave) return;

        if (havePulse_) {
            uint64_t measured = (nowUs >= lastPulseUs_) ? nowUs - lastPulseUs_ : 0;
            double f = filteredPulsePeriodUs_;
            // Reject glitch measurements (double-triggers, dropouts) but
            // still re-sync phase on the pulse itself.
            if (measured > 0.25 * f && measured < 4.0 * f && measured > 0) {
                filteredPulsePeriodUs_ = f + alpha_ * (static_cast<double>(measured) - f);
                tickPeriodUs_ = usPerSubdiv(filteredPulsePeriodUs_, subdivsPerPulse_);
            }
        } else {
            tickPeriodUs_ = usPerSubdiv(filteredPulsePeriodUs_, subdivsPerPulse_);
        }

        // Count snap: this pulse marks a pulse boundary, so snap the tick count
        // to the nearest multiple of subdivsPerPulse_. This deletes any
        // extra (or restores any missing) freewheeled subdiv ticks and is
        // what guarantees zero long-term drift: the count can never walk
        // away from the pulse train.
        {
            uint64_t S = static_cast<uint64_t>(subdivsPerPulse_);
            tickCount_ = ((tickCount_ + S / 2) / S) * S;
        }

        // Phase re-sync: the downbeat tick belongs on the pulse. If the
        // freewheeling grid already fired a tick within the last half
        // subdiv, treat that as the downbeat and carry the grid on;
        // otherwise fire the downbeat tick right now via advance().
        bool gridAlreadyFired =
            started_ && nowUs >= lastTickUs_ &&
            (nowUs - lastTickUs_) < tickPeriodUs_ / 2;
        if (gridAlreadyFired) {
            nextTickUs_ = lastTickUs_ + tickPeriodUs_;
        } else {
            nextTickUs_ = nowUs;
            started_ = true;
        }

        lastPulseUs_ = nowUs;
        havePulse_ = true;
    }

    // ---- transport ------------------------------------------------------
    // Restart pattern phase (pattern restarts on the next tick/pulse).
    // Measured slave tempo is kept.
    void onReset() {
        tickCount_ = 0;
        started_ = false;
        nextTickUs_ = 0;
        lastTickUs_ = 0;
    }

    // Advance the engine to nowUs. Returns how many 96-PPQ ticks became due.
    // The fired ticks are indices [tickCount() - n, tickCount()).
    int advance(uint64_t nowUs) {
        if (!started_) {
            nextTickUs_ = nowUs;
            started_ = true;
        }
        int n = 0;
        // Safety: bound a single catch-up burst (e.g. after a long stall).
        while (nextTickUs_ <= nowUs && n < 100000) {
            lastTickUs_ = nextTickUs_;
            nextTickUs_ += tickPeriodUs_;
            ++tickCount_;
            ++n;
        }
        return n;
    }

    // ---- state ----------------------------------------------------------
    uint64_t tickCount() const    { return tickCount_; }   // ticks emitted
    double   currentBpm() const {
        if (mode_ == Mode::Master) return bpm_;
        return 60000000.0 / (filteredPulsePeriodUs_ * externalPpq_);
    }
    Mode     mode() const         { return mode_; }
    uint64_t tickPeriodUs() const { return tickPeriodUs_; }
    int      subdivsPerPulse() const { return subdivsPerPulse_; }

private:
    static uint64_t usPerTickFromBpm(double bpm) {
        if (bpm <= 0.0) bpm = 120.0;
        uint64_t p = static_cast<uint64_t>(60000000.0 / (bpm * PPQ) + 0.5);
        return p > 0 ? p : 1;
    }
    static uint64_t usPerSubdiv(double pulsePeriodUs, int subdivs) {
        if (subdivs < 1) subdivs = 1;
        uint64_t p = static_cast<uint64_t>(pulsePeriodUs / subdivs + 0.5);
        return p > 0 ? p : 1;
    }

    Mode     mode_ = Mode::Master;
    double   bpm_ = 120.0;
    double   externalPpq_ = 4.0;
    int      subdivsPerPulse_ = PPQ / 4;  // 24 for 4-PPQ analog clock
    double   alpha_ = 0.25;               // slave period EMA coefficient

    bool     started_ = false;
    uint64_t tickCount_ = 0;      // ticks emitted so far
    uint64_t nextTickUs_ = 0;     // scheduled time of next tick
    uint64_t lastTickUs_ = 0;     // time the most recent tick fired
    uint64_t tickPeriodUs_ = 5208;

    bool     havePulse_ = false;
    uint64_t lastPulseUs_ = 0;
    double   filteredPulsePeriodUs_ = 500000.0;  // EMA of pulse period (µs)
};
