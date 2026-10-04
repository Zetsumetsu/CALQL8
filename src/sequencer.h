// sequencer.h — CALQL8 8-channel trigger sequencer core.
//
// Platform-independent: pure C++17, no hardware dependencies. Consumes the
// 96-PPQ tick stream from ClockEngine (call onTick() for every tick, in
// order) and produces trigger events.
//
// Per channel:
//  * clockDiv   — trigger every Nth 16th step (1 = every step)
//  * clockMult  — triggers per played step, evenly spaced (ratchet/burst)
//  * probability — per-channel probability 0..1
//  * stepProb[] — per-step probability multiplier 0..1
//  * stepGate[] — per-step on/off (the programmed pattern)
//  * euclidean  — when enabled, the pattern comes from the Euclidean
//                 generator (k hits / n steps / rotation) instead of stepGate
//  * swing      — 0..1, delays odd 16th steps by up to a 32nd note (12 ticks)
//
// Sub-tick timing (ratchet spacing, swing) is handled with a tiny pending
// event queue: onTick() fires anything due, then evaluates 16th boundaries.

#pragma once

#include <cstdint>
#include <vector>

// Deterministic RNG so host tests are repeatable. Seed it once at startup
// (in firmware, from an unconnected ADC or similar entropy source).
class XorShift32 {
public:
    explicit XorShift32(uint32_t seed = 0x12345678u) { this->seed(seed); }
    void seed(uint32_t s) { s_ = s ? s : 0x12345678u; }
    uint32_t next() {
        uint32_t x = s_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        s_ = x;
        return x;
    }
    // Uniform float in [0, 1).
    float nextFloat() { return (next() >> 8) * (1.0f / 16777216.0f); }
private:
    uint32_t s_;
};

class Sequencer {
public:
    static constexpr int NUM_CHANNELS = 8;
    static constexpr int MAX_STEPS = 16;
    static constexpr int TICKS_PER_16TH = 24;  // 96 PPQ / 4

    struct ChannelConfig {
        int   clockDiv = 1;        // play every Nth 16th (1..16)
        int   clockMult = 1;       // triggers per played step (1..8), ratchet
        float probability = 1.0f; // per-channel probability (0..1)
        float stepProb[MAX_STEPS]; // per-step probability multiplier (0..1)
        bool  stepGate[MAX_STEPS]; // per-step on/off pattern
        bool  euclidean = false;   // use Euclidean pattern instead of stepGate
        int   euclidHits = 4;      // k
        int   euclidSteps = 16;    // n
        int   euclidRotation = 0;
        float swing = 0.0f;        // 0..1, delays odd 16ths up to 12 ticks
        int   patternLength = 16;  // steps per pattern (1..16)

        ChannelConfig() {
            for (int i = 0; i < MAX_STEPS; ++i) {
                stepProb[i] = 1.0f;
                stepGate[i] = true;
            }
        }
    };

    struct TriggerEvent {
        int      channel;  // 0..7
        uint64_t tick;     // 96-PPQ tick index when it fired
    };

    Sequencer() = default;

    void setSeed(uint32_t seed) { rng_.seed(seed); }
    ChannelConfig& channel(int i) { return channels_[i]; }
    const ChannelConfig& channel(int i) const { return channels_[i]; }

    // Call once per engine tick, in increasing tick order.
    void onTick(uint64_t tick) {
        events_.clear();

        // Fire pending sub-tick events (ratchet tails, swung steps).
        for (size_t i = 0; i < pending_.size();) {
            if (pending_[i].tick <= tick) {
                events_.push_back(TriggerEvent{pending_[i].channel, tick});
                pending_[i] = pending_.back();
                pending_.pop_back();
            } else {
                ++i;
            }
        }

        if (tick % TICKS_PER_16TH != 0) return;  // only 16th boundaries

        uint64_t step16 = tick / TICKS_PER_16TH;
        for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
            const ChannelConfig& c = channels_[ch];
            if (c.patternLength < 1) continue;
            int plen = c.patternLength > MAX_STEPS ? MAX_STEPS : c.patternLength;
            int pstep = static_cast<int>(step16 % static_cast<uint64_t>(plen));

            bool gate = c.euclidean ? euclidHit(c, pstep) : c.stepGate[pstep];
            if (!gate) continue;

            int div = c.clockDiv < 1 ? 1 : c.clockDiv;
            if ((step16 % static_cast<uint64_t>(div)) != 0) continue;

            float p = c.probability * c.stepProb[pstep];
            if (p <= 0.0f) continue;
            // One roll per step: the whole step (all ratchet hits) lives or dies together.
            if (p < 1.0f && rng_.nextFloat() >= p) continue;

            uint64_t swingTicks =
                (pstep % 2 == 1) ? static_cast<uint64_t>(c.swing * 12.0f + 0.5f) : 0;
            int m = c.clockMult < 1 ? 1 : c.clockMult;
            for (int k = 0; k < m; ++k) {
                uint64_t t = tick + swingTicks +
                             static_cast<uint64_t>((k * TICKS_PER_16TH) / m);
                if (t == tick) {
                    events_.push_back(TriggerEvent{ch, tick});
                } else {
                    pending_.push_back(Pending{t, ch});
                }
            }
        }
    }

    // Trigger events produced by the most recent onTick() call.
    const std::vector<TriggerEvent>& events() const { return events_; }

    void reset() {
        pending_.clear();
        events_.clear();
    }

    // Euclidean hit test, exposed for unit tests.
    static bool euclidHit(const ChannelConfig& c, int step) {
        int n = c.euclidSteps;
        int k = c.euclidHits;
        if (n < 1) return false;
        if (k <= 0) return false;
        if (k >= n) return true;
        int idx = (step % n + n) % n;
        idx = (idx + c.euclidRotation) % n;
        // Bucket algorithm: distributes k hits as evenly as possible over n steps.
        return ((idx * k) % n) < k;
    }

private:
    struct Pending {
        uint64_t tick;
        int channel;
    };

    ChannelConfig channels_[NUM_CHANNELS];
    XorShift32 rng_;
    std::vector<Pending> pending_;
    std::vector<TriggerEvent> events_;
};
