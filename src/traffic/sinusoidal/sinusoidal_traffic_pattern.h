#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cmath>

class SinusoidalTrafficPattern : public ITrafficPattern {
public:
    SinusoidalTrafficPattern(double baseRateMbps,
                             double amplitudeMbps,
                             double frequencyHz);

    //Generate payload sized according to the current sinusoidal rate.
    std::vector<uint8_t> generatePayload(double intervalSec) override;

private:
    double baseRateMbps_;    //< base (mean) rate in Mbps
    double amplitudeMbps_;   //< amplitude of sine wave in Mbps
    double omega_;           //< angular frequency = 2pi·frequencyHz
    double elapsedTime_;     //< accumulated time in seconds
};
