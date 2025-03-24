#include "pure_burst_traffic_pattern.h"
#include "client_context.h"
#include <iostream>

PureBurstTrafficPattern::PureBurstTrafficPattern(ClientContext& context, SendFunction sendFunc)
    : AbstractBurstPattern(context, std::move(sendFunc))
{

    burst_interval_ms_ = static_cast<int>(context.scenario.burst_interval_sec * 1000);
    burst_duration_ms_ = static_cast<int>(context.scenario.burst_duration_sec * 1000);
    burst_bytes_total_ = context.scenario.burst_size_mb * 1'000'000 / 8;

    int burst_intervals = burst_duration_ms_ / interval_ms_;
    burst_bytes_per_interval_ = burst_intervals > 0 ? burst_bytes_total_ / burst_intervals : 0;
}

std::string PureBurstTrafficPattern::generatePayload() {
    std::string base_data(bytes_per_interval_, 'x');
    bool in_burst = (elapsed_ms_ % burst_interval_ms_) < burst_duration_ms_;

    if (in_burst) {
        std::string burst_data(burst_bytes_per_interval_, 'y');
        return base_data + burst_data;
    }
    return base_data;
}
