// metrics_types.h

#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

struct AggregateSample {
    std::chrono::system_clock::time_point timestamp;
    size_t bytesIn = 0;
    size_t bytesOut = 0;
    size_t packetsIn = 0;
    size_t packetsOut = 0;
    double throughputMbps = 0.0;
};


struct CPUUsageSnapshot {
    std::vector<double> perCoreUsages;
    double totalSum = 0.0;
    double selfCpu = 0.0;
    double selfShareVsTotal = 0.0;
    double selfShareVsSystemCap = 0.0;
};
