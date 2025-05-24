#pragma once

#include "cpu_usage_monitor.h"
#include "port_traffic_monitor.h"
#include <boost/asio.hpp>
#include <chrono>
#include <functional>

class UnifiedMonitorController {
public:
    using Clock = std::chrono::system_clock;
    using Timestamp = Clock::time_point;

    using Callback = std::function<void(
        Timestamp,
        const CPUUsageSnapshot &,
        const PortTrafficMonitor::AggregateSample &
    )>;

    UnifiedMonitorController(
        boost::asio::io_context &ioContext,
        std::shared_ptr<CPUUsageMonitor> cpuMonitor,
        std::shared_ptr<PortTrafficMonitor> networkMonitor,
        int intervalSec
    );

    void start(Callback callback);

    void stop();

private:
    void tick();

    boost::asio::io_context &ioContext_;
    std::shared_ptr<CPUUsageMonitor> cpuMonitor_;
    std::shared_ptr<PortTrafficMonitor> networkMonitor_;
    int intervalSec_;

    std::unique_ptr<boost::asio::steady_timer> timer_;
    Callback callback_;

    bool running_ = false;
};
