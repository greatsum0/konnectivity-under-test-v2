#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <unistd.h>  // for sysconf

/// Calls your callback every intervalSec with two samples:
///  - total CPU usage (%) across all cores
///  - this process’s CPU usage (%)
class CPUUsageMonitor {
public:
    /// cpuPctTotal, cpuPctSelf
    using Callback = std::function<void(double cpuPctTotal,
                                        double cpuPctSelf)>;

    /// @param intervalSec  how often (seconds) to report metrics
    explicit CPUUsageMonitor(double intervalSec);

    ~CPUUsageMonitor();

    /// Starts the background sampling thread.
    void start(Callback cb);

    /// Stops it (blocks until thread exits).
    void stop();

private:
    double readTotalCPUUsage() const;

    double readSelfCPUUsage() const;

    const double intervalSec_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};
    Callback callback_;

    // for total‐CPU delta computation
    mutable long long prevTotal_{0}, prevIdle_{0};

    // number of logical cores for per‐process %
#if __linux__
    const long coreCount_{sysconf(_SC_NPROCESSORS_ONLN)};
#endif

};
