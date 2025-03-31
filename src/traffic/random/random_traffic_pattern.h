#pragma once

#include "traffic_pattern.h"
#include "client_context.h"
#include <asio.hpp>
#include <functional>
#include <random>
#include <memory>

class RandomTrafficPattern : public ITrafficPattern, public std::enable_shared_from_this<RandomTrafficPattern> {
public:
    using SendFunction = std::function<void(const std::string&)>;

    RandomTrafficPattern(ClientContext& context, SendFunction sendFunc);

    void start() override;

private:
    void scheduleNextSend();

    ClientContext& context_;
    SendFunction sendFunc_;
    asio::steady_timer timer_;
    int interval_ms_;
    int elapsed_ms_;

    int max_rate_mbps_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> rate_dist_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point nextPlannedTime_;
};
