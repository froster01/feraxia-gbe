#include <cstdio>
#include "../../dll/dll/ping_tracker.h"
int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    const PingTracker::clock::time_point t0{};
    auto at = [&](int ms) { return t0 + std::chrono::milliseconds(ms); };

    { PingTracker p; check(!p.ms(at(0)).has_value(), "no sample before any pong"); }
    { PingTracker p; p.pong(at(40)); check(!p.ms(at(40)).has_value(), "pong without a sent ping is ignored"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); check(p.ms(at(40)) == 40, "first sample is the raw round trip"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); p.pong(at(60)); check(p.ms(at(60)) == 40, "duplicate pong from a second interface is ignored"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(2500)); check(!p.ms(at(2500)).has_value(), "reply slower than 2 s is not a sample"); }
    { PingTracker p; p.sent(at(0)); p.sent(at(1000)); p.pong(at(1030)); check(p.ms(at(1030)) == 30, "latest ping time is used"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); p.sent(at(1000)); p.pong(at(1080));
      check(p.ms(at(1080)) == 52, "second sample is smoothed (40 + 0.3 * (80 - 40))"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40));
      check(p.ms(at(40 + 15000)).has_value(), "sample still valid at 15 s");
      check(!p.ms(at(40 + 15001)).has_value(), "sample goes stale after 15 s"); }

    check(ping_tier(std::nullopt) == PingTier::none, "unknown ping has no tier");
    check(ping_tier(59) == PingTier::good, "59 ms is good");
    check(ping_tier(60) == PingTier::fair, "60 ms is fair");
    check(ping_tier(150) == PingTier::fair, "150 ms is fair");
    check(ping_tier(151) == PingTier::poor, "151 ms is poor");
    std::printf("%s: ping tracker\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
