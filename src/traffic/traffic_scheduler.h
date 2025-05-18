// traffic_scheduler.h
#pragma once

#include <asio.hpp>
#include "inetwork_client.h"  // your INetworkClient interface
#include <client_context.h>
#include <itraffic_pattern.h>

class TrafficScheduler {
public:
    /// rateMbps: target megabits/sec
    /// packetSize: size of each UDP/TCP packet in bytes
    /// durationSec: total run time
    TrafficScheduler(asio::io_context &io,
                     INetworkClient &client,
                     const ClientContext &context,
                     ITrafficPattern &pattern);

    /// kick off the async send loop
    void start();

private:
    void sendOne(const asio::error_code &ec);

    asio::io_context &io_;
    INetworkClient &client_;
    ClientContext context_;
    ITrafficPattern &pattern_;

    asio::steady_timer timer_;
    std::chrono::steady_clock::time_point nextTime_;
    std::chrono::steady_clock::time_point startTime_ =
        std::chrono::steady_clock::now();
    std::chrono::microseconds packetDelay_{};
    size_t maxPackets_{};
    size_t packetsSent_ = 0;
    std::vector<uint8_t> packetBuffer_;
};
