#include "ramp_down_traffic_pattern.h"
#include <iostream>

RampDownTrafficPattern::RampDownTrafficPattern(ClientContext& context, SendFunction sendFunc)
    : AbstractRampPattern(context, std::move(sendFunc)),
      starting_rate_mbps_(context.scenario.rate_mbps),
      ramp_down_duration_ms_(context.scenario.ramp_down_time * 1000)
{
}

int RampDownTrafficPattern::calculateRateMbps(int elapsed_ms) {
    if (elapsed_ms >= ramp_down_duration_ms_) {
        return 0;
    }

    // linear ramp down
    double progress = static_cast<double>(elapsed_ms) / ramp_down_duration_ms_;
    return static_cast<int>(starting_rate_mbps_ * (1.0 - progress));
}
