#pragma once

#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <metric_types.h>

class CPUUsageMonitor {
public:
    explicit CPUUsageMonitor(double intervalSec);
    CPUUsageSnapshot sample();

private:
    double intervalSec_;

#if __linux__
    std::vector<long long> prevTotal_;
    std::vector<long long> prevIdle_;
    int coreCount_;

    long long prevSelfJiffies_ = -1;
    std::chrono::steady_clock::time_point prevSelfTime_;

    std::vector<double> readPerCoreUsages(double &outSum);
    double readSelfCPUUsage();
#else
    std::vector<double> mockCoreUsages() const;
    double mockSelfUsage() const;
#endif
};
