// metrics_types.h

#pragma once

#include <chrono>
#include <cstddef>

struct AggregateSample {
    std::chrono::system_clock::time_point timestamp;
    size_t bytesIn = 0;
    size_t bytesOut = 0;
    size_t packetsIn = 0;
    size_t packetsOut = 0;
    double throughputMbps = 0.0;
};
