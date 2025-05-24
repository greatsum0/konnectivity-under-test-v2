#include "ramp_up_traffic_pattern.h"

RampUpTrafficPattern::RampUpTrafficPattern(const double targetRateMbps, const double rampTimeSec)
  : AbstractRampPattern(targetRateMbps, rampTimeSec)
{}

double RampUpTrafficPattern::currentRateFactor(const double elapsedSec) const {
    if (elapsedSec <= 0.0) {
        return 0.0;
    }
    if (elapsedSec >= rampTimeSec_) {
        return 1.0;
    }
    return elapsedSec / rampTimeSec_;
}
