#pragma once

#include "abstract_ramp_pattern.h"

class RampUpTrafficPattern : public AbstractRampPattern {
public:
    RampUpTrafficPattern(double maxRateMbps, double rampTimeSec);

protected:
    // Rate increases linearly until maxRate, then holds.
    double calculateRate(double elapsedSec) const override;
};
