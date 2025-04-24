#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <random>


class RandomBurstTrafficPattern : public ITrafficPattern {
public:

    RandomBurstTrafficPattern(unsigned seed,
                              double burstSizeMinMb,
                              double burstSizeMaxMb,
                              double burstDurationMinSec,
                              double burstDurationMaxSec,
                              double burstIntervalMinSec,
                              double burstIntervalMaxSec);

    std::vector<uint8_t> generatePayload(double intervalSec) override;

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> sizeDist_;
    std::uniform_real_distribution<double> durDist_;
    std::uniform_real_distribution<double> intDist_;

    double elapsed_;
    double nextBurstTime_;
    double currentBurstDuration_;
};
