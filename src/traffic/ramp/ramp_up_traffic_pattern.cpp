#include "ramp_up_traffic_pattern.h"
#include <iostream>

RampUpTrafficPattern::RampUpTrafficPattern(ClientContext& context, SendFunction sendFunc)
    : AbstractRampPattern(context, std::move(sendFunc)),
      target_rate_mbps_(context.scenario.rate_mbps),
      ramp_up_duration_ms_(context.scenario.ramp_up_time * 1000)
{
}

int RampUpTrafficPattern::calculateRateMbps(int elapsed_ms) {
    if (elapsed_ms >= ramp_up_duration_ms_) {
        return target_rate_mbps_;
    }

    // Linear ramp up
    double progress = static_cast<double>(elapsed_ms) / ramp_up_duration_ms_;
    return static_cast<int>(target_rate_mbps_ * progress);
}
