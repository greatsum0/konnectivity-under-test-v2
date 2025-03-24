#pragma once

#include "abstract_burst_pattern.h"
#include <random>

class RandomBurstTrafficPattern : public AbstractBurstPattern {
public:
    using SendFunction = std::function<void(const std::string &)>;

    RandomBurstTrafficPattern(ClientContext &context, SendFunction sendFunc);

protected:
    std::string generatePayload() override;

private:
    std::mt19937 rng_;  // random number generator
    std::uniform_int_distribution<int> burst_size_dist_;
    std::uniform_int_distribution<int> burst_duration_dist_;
    std::uniform_int_distribution<int> burst_interval_dist_;

    int current_burst_duration_ms_ = 0;
    int current_burst_elapsed_ms_ = 0;
    int time_to_next_burst_ms_ = 0;
    bool in_burst_ = false;
};
