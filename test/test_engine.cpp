// test_engine.cpp — host unit tests for the CALQL8 clock engine + sequencer.
// Build & run:  make test
// Pure C++17, no hardware. Deterministic (seeded RNG, fixed jitter vectors).

#include <cstdio>
#include <cmath>
#include <vector>

#include "../src/clock_engine.h"
#include "../src/sequencer.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        ++failures; \
        std::printf("  FAIL line %d: %s\n", __LINE__, #cond); \
    } \
} while (0)

#define SECTION(name) std::printf("[%s]\n", name)

using Event = Sequencer::TriggerEvent;

// Advance the engine from fromUs..toUs in stepUs increments, feeding every
// tick to the sequencer and collecting trigger events.
static void runTicks(ClockEngine& eng, Sequencer& seq,
                     uint64_t fromUs, uint64_t toUs, uint64_t stepUs,
                     std::vector<Event>& out) {
    for (uint64_t t = fromUs; t <= toUs; t += stepUs) {
        uint64_t base = eng.tickCount();
        int n = eng.advance(t);
        for (int i = 0; i < n; ++i) {
            seq.onTick(base + i);
            for (const auto& e : seq.events()) out.push_back(e);
        }
    }
}

// Silence all channels except ch.
static void soloChannel(Sequencer& seq, int ch) {
    for (int i = 0; i < Sequencer::NUM_CHANNELS; ++i)
        seq.channel(i).probability = (i == ch) ? 1.0f : 0.0f;
}

// Deterministic pseudo-jitter in [-2000, +2000] µs.
static int64_t jitterUs(uint64_t i) {
    return static_cast<int64_t>((i * 2654435761u) % 4001u) - 2000;
}

static void test_master_tick_count() {
    SECTION("master tick count");
    ClockEngine eng;
    eng.setMaster(120.0);
    CHECK(eng.currentBpm() == 120.0);
    // 120 BPM -> 192 ticks/sec. Simulate 10 s in 1 ms steps.
    const uint64_t totalUs = 10ULL * 1000000ULL;
    for (uint64_t t = 0; t <= totalUs; t += 1000) eng.advance(t);
    uint64_t period = eng.tickPeriodUs();  // 5208
    uint64_t expected = totalUs / period + 1;  // inclusive endpoints
    std::printf("  ticks=%llu expected=%llu (period=%lluus)\n",
                (unsigned long long)eng.tickCount(),
                (unsigned long long)expected,
                (unsigned long long)period);
    CHECK(eng.tickCount() == expected);
}

static void test_slave_no_drift() {
    SECTION("slave mode: no accumulated drift with jittered input");
    ClockEngine eng;
    eng.setSlave(4.0);  // 4 PPQ analog clock
    const int NPULSE = 501;
    const uint64_t NOMINAL = 125000;  // 120 BPM, 4 PPQ
    uint64_t t = 0;
    eng.onClockPulse(0);
    eng.advance(0);
    for (int i = 1; i < NPULSE; ++i) {
        uint64_t tp = (uint64_t)i * NOMINAL + (uint64_t)( (int64_t)jitterUs(i) );
        // main-loop style: advance in 1 ms steps, then handle the pulse
        for (uint64_t ta = t + 1000; ta < tp; ta += 1000) eng.advance(ta);
        eng.advance(tp);
        eng.onClockPulse(tp);
        eng.advance(tp);
        t = tp;
    }
    // 24 subdivs per pulse; first pulse emitted 1 tick, each of the
    // remaining NPULSE-1 intervals contributes ~24 ticks.
    uint64_t expected = 1 + (uint64_t)(NPULSE - 1) * 24;
    int64_t err = (int64_t)eng.tickCount() - (int64_t)expected;
    std::printf("  ticks=%llu expected=%llu err=%lld\n",
                (unsigned long long)eng.tickCount(),
                (unsigned long long)expected, (long long)err);
    CHECK(err > -15 && err < 15);
    double bpm = eng.currentBpm();
    std::printf("  measured bpm=%.3f\n", bpm);
    CHECK(std::fabs(bpm - 120.0) < 0.5);

    // Flywheel: pulses stop, engine keeps freewheeling at last tempo.
    uint64_t before = eng.tickCount();
    for (uint64_t ta = t + 1000; ta <= t + 1000000; ta += 1000) eng.advance(ta);
    uint64_t freeTicks = eng.tickCount() - before;
    std::printf("  flywheel ticks in 1s: %llu\n", (unsigned long long)freeTicks);
    CHECK(freeTicks > 180 && freeTicks < 204);  // ~192 expected
}

static void test_divider() {
    SECTION("per-channel clock divider");
    ClockEngine eng;
    eng.setMaster(120.0);
    Sequencer seq;
    soloChannel(seq, 0);
    seq.channel(0).clockDiv = 4;
    std::vector<Event> ev;
    runTicks(eng, seq, 0, 1999000, 1000, ev);  // 16 sixteenth-steps (stop before step 16)
    std::vector<int> steps;
    for (auto& e : ev) {
        CHECK(e.channel == 0);
        steps.push_back((int)(e.tick / 24));
    }
    std::printf("  triggers at 16th steps:");
    for (int s : steps) std::printf(" %d", s);
    std::printf("\n");
    CHECK(steps.size() == 4);
    if (steps.size() == 4) {
        CHECK(steps[0] == 0 && steps[1] == 4 && steps[2] == 8 && steps[3] == 12);
    }
}

static void test_ratchet() {
    SECTION("ratchet / multiplier sub-trigger counts");
    ClockEngine eng;
    eng.setMaster(120.0);
    Sequencer seq;
    soloChannel(seq, 0);
    seq.channel(0).clockMult = 4;
    std::vector<Event> ev;
    // 2 sixteenth steps = 48 ticks; stop before tick 48 fires.
    runTicks(eng, seq, 0, 249000, 1000, ev);
    std::vector<uint64_t> ticks;
    for (auto& e : ev) ticks.push_back(e.tick);
    std::printf("  trigger ticks:");
    for (auto tk : ticks) std::printf(" %llu", (unsigned long long)tk);
    std::printf("\n");
    const uint64_t want[] = {0, 6, 12, 18, 24, 30, 36, 42};
    CHECK(ticks.size() == 8);
    for (size_t i = 0; i < ticks.size() && i < 8; ++i) CHECK(ticks[i] == want[i]);
}

static void test_euclidean() {
    SECTION("euclidean pattern generator");
    // Known vector: E(3,8) -> hits on steps 0, 3, 6.
    {
        ClockEngine eng;
        eng.setMaster(120.0);
        Sequencer seq;
        soloChannel(seq, 0);
        auto& c = seq.channel(0);
        c.euclidean = true;
        c.euclidHits = 3;
        c.euclidSteps = 8;
        c.euclidRotation = 0;
        c.patternLength = 8;
        std::vector<Event> ev;
        runTicks(eng, seq, 0, 999000, 1000, ev);  // 8 steps, stop before step 8
        std::vector<int> steps;
        for (auto& e : ev) steps.push_back((int)(e.tick / 24));
        std::printf("  E(3,8) steps:");
        for (int s : steps) std::printf(" %d", s);
        std::printf("\n");
        CHECK(steps.size() == 3);
        if (steps.size() == 3)
            CHECK(steps[0] == 0 && steps[1] == 3 && steps[2] == 6);
    }
    // Hit counts over a full cycle.
    {
        Sequencer seq;
        auto& c = seq.channel(0);
        c.euclidSteps = 16; c.euclidHits = 4;
        int n = 0;
        for (int s = 0; s < 16; ++s) n += Sequencer::euclidHit(c, s) ? 1 : 0;
        CHECK(n == 4);
        c.euclidSteps = 8; c.euclidHits = 5;
        n = 0;
        for (int s = 0; s < 8; ++s) n += Sequencer::euclidHit(c, s) ? 1 : 0;
        CHECK(n == 5);
        // Rotation preserves the hit count.
        c.euclidRotation = 3;
        n = 0;
        for (int s = 0; s < 8; ++s) n += Sequencer::euclidHit(c, s) ? 1 : 0;
        CHECK(n == 5);
    }
}

static void test_probability() {
    SECTION("probability: bounds and determinism");
    // p = 0 -> silence
    {
        ClockEngine eng;
        eng.setMaster(120.0);
        Sequencer seq;
        soloChannel(seq, 0);
        seq.channel(0).probability = 0.0f;
        std::vector<Event> ev;
        runTicks(eng, seq, 0, 2000000, 1000, ev);
        CHECK(ev.empty());
    }
    // seeded runs are deterministic
    auto run = [](uint32_t seed) {
        ClockEngine eng;
        eng.setMaster(120.0);
        Sequencer seq;
        seq.setSeed(seed);
        soloChannel(seq, 0);
        seq.channel(0).probability = 0.5f;
        std::vector<Event> ev;
        runTicks(eng, seq, 0, 8000000, 1000, ev);  // 64 steps
        std::vector<uint64_t> ticks;
        for (auto& e : ev) ticks.push_back(e.tick);
        return ticks;
    };
    auto a = run(42);
    auto b = run(42);
    auto c = run(7);
    std::printf("  seed42 triggers=%zu seed7 triggers=%zu\n", a.size(), b.size());
    CHECK(a == b);            // deterministic
    CHECK(a != c);            // seed actually matters
    CHECK(!a.empty() && a.size() < 64);  // 0 < p < 1 behaves sanely
}

static void test_swing() {
    SECTION("swing delays odd 16ths");
    ClockEngine eng;
    eng.setMaster(120.0);
    Sequencer seq;
    soloChannel(seq, 0);
    seq.channel(0).swing = 1.0f;  // full swing = +12 ticks on odd 16ths
    std::vector<Event> ev;
    runTicks(eng, seq, 0, 499000, 1000, ev);  // 4 steps, stop before step 4
    std::vector<uint64_t> ticks;
    for (auto& e : ev) ticks.push_back(e.tick);
    std::printf("  swung trigger ticks:");
    for (auto tk : ticks) std::printf(" %llu", (unsigned long long)tk);
    std::printf("\n");
    const uint64_t want[] = {0, 36, 48, 84};
    CHECK(ticks.size() == 4);
    for (size_t i = 0; i < ticks.size() && i < 4; ++i) CHECK(ticks[i] == want[i]);
}

static void test_reset() {
    SECTION("reset restarts phase");
    ClockEngine eng;
    eng.setMaster(120.0);
    for (uint64_t t = 0; t <= 1000000; t += 1000) eng.advance(t);
    CHECK(eng.tickCount() > 0);
    eng.onReset();
    CHECK(eng.tickCount() == 0);
    eng.advance(2000000);
    CHECK(eng.tickCount() > 0);
}

int main() {
    test_master_tick_count();
    test_slave_no_drift();
    test_divider();
    test_ratchet();
    test_euclidean();
    test_probability();
    test_swing();
    test_reset();
    std::printf("\n%d checks, %d failures: %s\n",
                checks, failures, failures == 0 ? "ALL PASS" : "FAILURES");
    return failures == 0 ? 0 : 1;
}
