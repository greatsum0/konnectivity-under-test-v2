#include "ramp_up_traffic_pattern.h"

RampUpTrafficPattern::RampUpTrafficPattern(double maxRateMbps, double rampTimeSec)
  : AbstractRampPattern(maxRateMbps, rampTimeSec)
{}

double RampUpTrafficPattern::calculateRate(double elapsedSec) const {
    if (elapsedSec < rampTimeSec_) {
        // ramp from 0 → maxRate
        return maxRateMbps_ * (elapsedSec / rampTimeSec_);
    }
    // hold at maxRate
    return maxRateMbps_;
}
