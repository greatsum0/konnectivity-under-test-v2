#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <random>

class RandomBurstTrafficPattern : public ITrafficPattern {
public:
    RandomBurstTrafficPattern(unsigned seed,
                              double rateMbps,
                              double burstSizeMinMb,
                              double burstSizeMaxMb,
                              double burstDurationMinMs,
                              double burstDurationMaxMs,
                              double burstIntervalMinMs,
                              double burstIntervalMaxMs);

    std::vector<uint8_t> generatePayload(double intervalSec) override;

private:
    double rateMbps_;

    std::mt19937 rng_;
    std::uniform_real_distribution<> sizeDist_; // bytes
    std::uniform_real_distribution<> durDist_; // seconds
    std::uniform_real_distribution<> intDist_; // seconds

    double elapsed_;
    double nextBurstTime_;
    double currentBurstDuration_;
    size_t remainingBurstBytes_;
};
