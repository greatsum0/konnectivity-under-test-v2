#include "random_burst_traffic_pattern.h"
#include <iostream>

#include "client_context.h"

RandomBurstTrafficPattern::RandomBurstTrafficPattern(ClientContext &context, SendFunction sendFunc)
    : AbstractBurstPattern(context, std::move(sendFunc)) {
    const int seed = context.scenario.random_seed;
    rng_ = std::mt19937(seed);


    // Convert burst size in MB to bytes
    burst_size_dist_ = std::uniform_int_distribution<int>(context.scenario.min_burst_size_mb,
                                                          context.scenario.max_burst_size_mb);
    burst_duration_dist_ = std::uniform_int_distribution<int>(context.scenario.min_burst_duration_ms,
                                                              context.scenario.max_burst_duration_ms);
    burst_interval_dist_ = std::uniform_int_distribution<int>(context.scenario.min_burst_interval_ms,
                                                              context.scenario.max_burst_interval_ms);

    // set the first burst time
    time_to_next_burst_ms_ = burst_interval_dist_(rng_);
}

std::string RandomBurstTrafficPattern::generatePayload() {
    std::string payload;

    if (in_burst_) {
        if (current_burst_elapsed_ms_ >= current_burst_duration_ms_) {
            // burst ended
            in_burst_ = false;
            current_burst_elapsed_ms_ = 0;
            time_to_next_burst_ms_ = burst_interval_dist_(rng_);
        } else {
            // when the burst is active, send data
            int burst_bytes = burst_size_dist_(rng_) * 1'000'000 / 8; // MB to bytes
            payload = std::string(burst_bytes, 'r');


            current_burst_elapsed_ms_ += interval_ms_;
        }
    } else {
        if (time_to_next_burst_ms_ <= 0) {
            // new burst started
            in_burst_ = true;
            current_burst_duration_ms_ = burst_duration_dist_(rng_);
            current_burst_elapsed_ms_ = 0;
        } else {
            time_to_next_burst_ms_ -= interval_ms_;
        }
    }

    return payload;
}
