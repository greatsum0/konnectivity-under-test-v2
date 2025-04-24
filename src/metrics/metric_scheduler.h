#pragma once

#include "metric_collector.h"
#include <asio.hpp>
#include <chrono>

class MetricScheduler {
public:
    MetricScheduler(asio::io_context &io,
                    MetricCollector &collector,
                    double intervalSec);

    void start();

private:
    void scheduleNext();

    asio::steady_timer timer_;
    MetricCollector &collector_;
    std::chrono::steady_clock::time_point nextTime_;
    std::chrono::duration<double> interval_;
};
