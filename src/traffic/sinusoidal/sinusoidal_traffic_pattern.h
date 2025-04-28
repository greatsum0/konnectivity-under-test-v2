#pragma once

#include "itraffic_pattern.h"
#include <chrono>
#include <cstddef>

class SinusoidalTrafficPattern final : public ITrafficPattern {
public:
    /// @param meanRateMbps   the center rate (Mbps) around which you oscillate
    /// @param amplitudeMbps  how far above/below mean you go (± Mbps)
    /// @param frequencyHz    cycles per second (1 / period)
    SinusoidalTrafficPattern(double meanRateMbps,
                             double amplitudeMbps,
                             double frequencyHz);

    // Estimate total packets based on the *mean* rate
    [[nodiscard]] std::size_t computeMaxPackets(double durationSec, size_t packetSize) const override;

    // Compute per‐send delay based on instantaneous sine rate
    std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) override;

private:
    // instantaneous rate (Mbps) at t seconds since start
    [[nodiscard]] double currentRateMbps(double elapsedSec) const;

    const double meanRateMbps_;
    const double amplitudeMbps_;
    const double frequencyHz_;
    const std::chrono::steady_clock::time_point startTime_;
};
