// traffic_scheduler.h
#pragma once

#include <asio.hpp>
#include "inetwork_client.h"  // your INetworkClient interface
#include <chrono>

class TrafficScheduler {
public:
    /// rateMbps: target megabits/sec
    /// packetSize: size of each UDP/TCP packet in bytes
    /// durationSec: total run time
    TrafficScheduler(asio::io_context& io,
                     INetworkClient& client,
                     double rateMbps,
                     size_t packetSize,
                     double durationSec);

    /// kick off the async send loop
    void start();

private:
    void sendOne(const asio::error_code& ec);

    asio::io_context&                       io_;
    asio::steady_timer                      timer_;
    INetworkClient&                         client_;
    std::chrono::steady_clock::time_point   nextTime_;
    std::chrono::microseconds               packetDelay_;
    size_t                                  maxPackets_;
    size_t                                  packetsSent_ = 0;
    std::vector<uint8_t>                    packetBuffer_;
};
