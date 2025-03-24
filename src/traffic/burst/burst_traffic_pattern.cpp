#include "burst_traffic_pattern.h"

#include <iostream>

#include "client_context.h"

BurstTrafficPattern::BurstTrafficPattern(ClientContext &context, SendFunction sendFunc)
    : AbstractBurstPattern(context, std::move(sendFunc)) {
    const double base_bytes_per_sec = (context.rateMbps() * 1'000'000) / 8.0;
    bytes_per_interval_ = static_cast<int>((base_bytes_per_sec * interval_ms_) / 1000.0);

    burst_interval_ms_ = static_cast<int>(context.scenario.burst_interval_sec * 1000);
    burst_duration_ms_ = static_cast<int>(context.scenario.burst_duration_sec * 1000);
    burst_bytes_total_ = context.scenario.burst_size_mb * 1'000'000 / 8;

    int burst_intervals = burst_duration_ms_ / interval_ms_;
    burst_bytes_per_interval_ = burst_intervals > 0 ? burst_bytes_total_ / burst_intervals : 0;
}

std::string BurstTrafficPattern::generatePayload() {
    std::string base_data(bytes_per_interval_, 'x');
    bool in_burst = (elapsed_ms_ % burst_interval_ms_) < burst_duration_ms_;

    if (in_burst) {
        std::string burst_data(burst_bytes_per_interval_, 'y');
        return base_data + burst_data;
    }
    return base_data;
}
