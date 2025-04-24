#pragma once

#include "inetwork_client.h"
#include "itraffic_pattern.h"
#include <asio.hpp>
#include <chrono>
#include <cstddef>

class TrafficScheduler {
public:
    TrafficScheduler(asio::io_context &io,
                     INetworkClient &client,
                     ITrafficPattern &pattern,
                     double intervalSec,
                     double durationSec);

    void start();

private:
    void scheduleNext();

    asio::io_context &io_;
    asio::steady_timer timer_;
    INetworkClient &client_;
    ITrafficPattern &pattern_;
    std::chrono::steady_clock::time_point nextTime_;
    std::size_t maxIterations_;
    std::size_t currentIteration_;
    std::chrono::duration<double> interval_;
};
