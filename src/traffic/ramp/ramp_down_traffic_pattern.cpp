#include "ramp_down_traffic_pattern.h"

RampDownTrafficPattern::RampDownTrafficPattern(double maxRateMbps, double rampTimeSec)
  : AbstractRampPattern(maxRateMbps, rampTimeSec)
{}

double RampDownTrafficPattern::calculateRate(double elapsedSec) const {
    if (elapsedSec < rampTimeSec_) {
        // ramp from maxRate → 0
        return maxRateMbps_ * (1.0 - (elapsedSec / rampTimeSec_));
    }
    // hold at 0
    return 0.0;
}
