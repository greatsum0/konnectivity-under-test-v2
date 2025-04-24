#pragma once

#include "abstract_ramp_pattern.h"

class RampDownTrafficPattern : public AbstractRampPattern {
public:
    RampDownTrafficPattern(double maxRateMbps, double rampTimeSec);

protected:
    // Rate decreases linearly from maxRate → 0, then holds at 0.
    double calculateRate(double elapsedSec) const override;
};
