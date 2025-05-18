#pragma once

#include "abstract_burst_pattern.h"
#include <cstddef>

/// Steady mean rate plus periodic fixed-size bursts.
class BurstTrafficPattern : public AbstractBurstPattern {
public:
    BurstTrafficPattern(double meanRateMbps,
                        double burstSizeMB,
                        double burstDurationSec,
                        double burstIntervalSec);

    std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) override;

    [[nodiscard]] std::size_t computeMaxPackets(double durationSec, size_t packetSize) const override;

private:
    double meanRateMbps_;
    double burstSizeMB_;
    double burstDurationSec_;
    double burstIntervalSec_;
};
