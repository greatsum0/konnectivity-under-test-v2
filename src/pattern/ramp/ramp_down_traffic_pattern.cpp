#include "ramp_down_traffic_pattern.h"


RampDownTrafficPattern::RampDownTrafficPattern(const double targetRateMbps, const double rampTimeSec)
  : AbstractRampPattern(targetRateMbps, rampTimeSec)
{}

double RampDownTrafficPattern::currentRateFactor(const double elapsedSec) const {
    if (elapsedSec <= 0.0) {
        return 1.0;
    }
    if (elapsedSec >= rampTimeSec_) {
        return 0.0;
    }
    return 1.0 - (elapsedSec / rampTimeSec_);
}
