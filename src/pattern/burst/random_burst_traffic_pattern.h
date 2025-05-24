#pragma once

#include "abstract_burst_pattern.h"
#include <random>
#include <cstddef>

/// Random bursts on top of a base rate.
class RandomBurstTrafficPattern final : public AbstractBurstPattern {
public:
    RandomBurstTrafficPattern(double meanRateMbps,
                              double minBurstSizeMB,
                              double maxBurstSizeMB,
                              double minBurstDurationMs,
                              double maxBurstDurationMs,
                              double minBurstIntervalMs,
                              double maxBurstIntervalMs,
                              unsigned int seed);

    std::size_t computeMaxPackets(double durationSec, std::size_t packetSize) const override;

    std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) override;

private:
    double meanRateMbps_;
    double minBurstSizeMB_, maxBurstSizeMB_;
    double minBurstDurationMs_, maxBurstDurationMs_;
    double minBurstIntervalMs_, maxBurstIntervalMs_;
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<double> sizeDist_;
    mutable std::uniform_real_distribution<double> durDist_;
    mutable std::uniform_real_distribution<double> intDist_;
    mutable double nextBurstStartSec_{0.0};
    mutable double currentBurstEndSec_{0.0};
    mutable double currentBurstRateMbps_{0.0};
};
