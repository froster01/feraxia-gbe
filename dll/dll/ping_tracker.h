#pragma once
#include <chrono>
#include <cmath>
#include <optional>

// Times the existing PING -> PONG UDP exchange for one connection.
// Pure logic: no sockets, no locks. Owned and used by the networking thread only.
struct PingTracker {
    using clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds max_sample{2000};
    static constexpr std::chrono::seconds stale_after{15};
    static constexpr double smoothing = 0.3;

    // Call right before a PING is sent to this connection. Later calls overwrite earlier ones.
    void sent(clock::time_point now) {
        sent_at = now;
        awaiting = true;
    }

    // Call when a PONG arrives from this connection.
    void pong(clock::time_point now) {
        if (!awaiting) return;
        awaiting = false;
        const auto elapsed = now - sent_at;
        if (elapsed < clock::duration::zero() || elapsed > max_sample) return;
        const double sample = std::chrono::duration<double, std::milli>(elapsed).count();
        smoothed_ms = has_sample ? smoothed_ms + smoothing * (sample - smoothed_ms) : sample;
        has_sample = true;
        last_sample_at = now;
    }

    std::optional<int> ms(clock::time_point now) const {
        if (!has_sample || now - last_sample_at > stale_after) return std::nullopt;
        return static_cast<int>(std::lround(smoothed_ms));
    }

private:
    clock::time_point sent_at{};
    clock::time_point last_sample_at{};
    double smoothed_ms = 0.0;
    bool awaiting = false;
    bool has_sample = false;
};

enum class PingTier { none, good, fair, poor };

inline PingTier ping_tier(std::optional<int> ms) {
    if (!ms.has_value()) return PingTier::none;
    if (*ms < 60) return PingTier::good;
    if (*ms <= 150) return PingTier::fair;
    return PingTier::poor;
}
