// src/traffic_scheduler.h
#pragma once

#include <boost/asio.hpp>
#include "inetwork_client.h"
#include "client_context.h"
#include "itraffic_pattern.h"
#include <vector>
#include <chrono>

class TrafficScheduler {
public:
    TrafficScheduler(boost::asio::io_context &io,
                     INetworkClient &client,
                     const ClientContext &context,
                     ITrafficPattern &pattern,
                     std::function<void()> onFinish = {});

    /// Kick off the async send loop
    void start();

private:
    void sendOne(const boost::system::error_code &ec);

    boost::asio::io_context &io_;
    INetworkClient &client_;
    ClientContext context_;
    ITrafficPattern &pattern_;

    boost::asio::steady_timer timer_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point nextTime_;

    std::vector<uint8_t> packetBuffer_;
    size_t maxPackets_{};
    size_t packetsSent_{0};
    std::function<void()> onFinish_;
};
