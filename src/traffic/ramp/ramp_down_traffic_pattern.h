#pragma once

#include "abstract_ramp_pattern.h"

class RampDownTrafficPattern : public AbstractRampPattern {
public:
    using SendFunction = std::function<void(const std::string&)>;

    RampDownTrafficPattern(ClientContext& context, SendFunction sendFunc);

protected:
    int calculateRateMbps(int elapsed_ms) override;

private:
    int starting_rate_mbps_;
    int ramp_down_duration_ms_;
};
