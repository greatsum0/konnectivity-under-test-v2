#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>

// #if __linux__
#include <unistd.h>
// #endif

class CPUUsageMonitor {
public:
    using ExtendedCallback = std::function<void(
        double cpu0, double cpu1, double cpu2, double cpu3,
        double totalCoreUsage,
        double selfCpu,
        double selfShareVsTotal,
        double selfShareVsSystemCap)>;

    explicit CPUUsageMonitor(double intervalSec);

    ~CPUUsageMonitor();

    void start(ExtendedCallback cb);

    void stop();

private:
    std::vector<double> readPerCoreUsages(double &outSum) const;

    double readSelfCPUUsage() const;

    const double intervalSec_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};
    ExtendedCallback callback_;

    mutable std::vector<long long> prevTotal_;
    mutable std::vector<long long> prevIdle_;

#if __linux__
    const long coreCount_;
#else
    const long coreCount_ = 4;
#endif
};
