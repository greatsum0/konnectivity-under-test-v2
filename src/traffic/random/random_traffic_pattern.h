#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <random>
#include <cmath>

class RandomTrafficPattern : public ITrafficPattern {
public:

    RandomTrafficPattern(unsigned seed, double maxRateMbps);

    //Generate a payload sized according to a random rate over intervalSec.
    std::vector<uint8_t> generatePayload(double intervalSec) override;

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> distRate_;
    double maxRateMbps_;
};
