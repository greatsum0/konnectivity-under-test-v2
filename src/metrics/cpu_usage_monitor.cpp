#include "cpu_usage_monitor.h"

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

CPUUsageMonitor::CPUUsageMonitor(double intervalSec)
    : intervalSec_(intervalSec) {
}

CPUUsageMonitor::~CPUUsageMonitor() {
    stop();
}

void CPUUsageMonitor::start(Callback cb) {
    if (running_) return;
    callback_ = std::move(cb);
    running_ = true;

    timerThread_ = std::thread([this]() {
        using namespace std::chrono;
        auto next = steady_clock::now() + duration<double>(intervalSec_);
        while (running_) {
            std::this_thread::sleep_until(next);
            next += duration<double>(intervalSec_);

            double totalCpu = readTotalCPUUsage();
            double selfCpu = readSelfCPUUsage();
            if (callback_) callback_(totalCpu, selfCpu);
        }
    });
}

void CPUUsageMonitor::stop() {
    if (!running_) return;
    running_ = false;
    if (timerThread_.joinable()) timerThread_.join();
}

double CPUUsageMonitor::readTotalCPUUsage() const {
#if __linux__
    std::ifstream statFile("/proc/stat");
    std::string line;
    std::getline(statFile, line);
    std::istringstream ss(line);

    std::string cpuLabel;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpuLabel
            >> user >> nice >> system >> idle
            >> iowait >> irq >> softirq >> steal;

    long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long long used = total - idle;

    long long deltaTotal = total - prevTotal_;
    long long deltaIdle = idle - prevIdle_;
    long long deltaUsed = deltaTotal - deltaIdle;

    prevTotal_ = total;
    prevIdle_ = idle;

    // first compute “percent busy” over all CPUs…
    double percentBusy = (deltaTotal > 0)
                     ? (static_cast<double>(deltaUsed) * 100.0 / static_cast<double>(deltaTotal))
                     : 0.0;
    // …then multiply by coreCount_ to get total‐core percentage
    return percentBusy * static_cast<double>(coreCount_);
#else
    return 5.0;
#endif
}

double CPUUsageMonitor::readSelfCPUUsage() const {
#if __linux__
    std::ifstream selfFile("/proc/self/stat");
    std::string line;
    std::getline(selfFile, line);
    std::istringstream selfStream(line);

    std::string token;
    long utime = 0, stime = 0;
    // fields: pid(1) comm(2) state(3) ppid(4) pgrp(5) session(6)
    //         tty_nr(7) tpgid(8) flags(9) minflt(10) cminflt(11)
    //         majflt(12) cmajflt(13) utime(14) stime(15)
    for (int i = 1; i <= 15; ++i) {
        selfStream >> token;
        if (i == 14) utime = std::stol(token);
        if (i == 15) stime = std::stol(token);
    }

    static long long previousJiffies = 0;
    long long currentJiffies = utime + stime;
    long long deltaJiffies = currentJiffies - previousJiffies;
    previousJiffies = currentJiffies;

    static auto previousTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> wallElapsed = currentTime - previousTime;
    previousTime = currentTime;

    const double jiffiesPerSec = static_cast<double>(sysconf(_SC_CLK_TCK));
    if (wallElapsed.count() <= 0.0) return 0.0;

    // CPU% = (process jiffies / (jiffies/sec * elapsedSec)) normalized by core count
    return (static_cast<double>(deltaJiffies) * 100.0 * coreCount_)
           / (jiffiesPerSec * wallElapsed.count());
#else
    return 6.0;
#endif
}
