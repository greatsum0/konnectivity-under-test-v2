#pragma once

#include "abstract_ramp_pattern.h"

class RampUpTrafficPattern : public AbstractRampPattern {
public:
    using SendFunction = std::function<void(const std::string&)>;

    RampUpTrafficPattern(ClientContext& context, SendFunction sendFunc);

protected:
    int calculateRateMbps(int elapsed_ms) override;

private:
    int target_rate_mbps_;
    int ramp_up_duration_ms_;
};
