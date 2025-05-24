#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>
#include <mutex>
#include <condition_variable>

#if __linux__
#include <unistd.h>
#endif

struct CPUUsageSnapshot {
    std::vector<double> perCoreUsages;
    double totalSum;
    double selfCpu;
    double selfShareVsTotal;
    double selfShareVsSystemCap;
};

class CPUUsageMonitor {
public:
    using ExtendedCallback = std::function<void(const CPUUsageSnapshot &)>;

    explicit CPUUsageMonitor(double intervalSec);

    ~CPUUsageMonitor();

    void start(ExtendedCallback cb);

    void stop();

    CPUUsageSnapshot sample();

private:
    std::vector<double> readPerCoreUsages(double &outSum);

    double readSelfCPUUsage();

    const double intervalSec_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};
    ExtendedCallback callback_;

    std::mutex mtx_;
    std::condition_variable cv_;

#if __linux__
    long coreCount_;
    std::vector<long long> prevTotal_;
    std::vector<long long> prevIdle_;
    long long prevSelfJiffies_ = -1;
    std::chrono::steady_clock::time_point prevSelfTime_;
#endif
};
