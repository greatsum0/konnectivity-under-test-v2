#include "cpu_usage_monitor.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <chrono>
#include <unistd.h>

CPUUsageMonitor::CPUUsageMonitor(double intervalSec)
    : intervalSec_(intervalSec)
#if __linux__
    , coreCount_(sysconf(_SC_NPROCESSORS_ONLN)),
      prevTotal_(coreCount_, 0),
      prevIdle_(coreCount_, 0),
      prevSelfTime_(std::chrono::steady_clock::now())
#endif
{
}

CPUUsageSnapshot CPUUsageMonitor::sample() {
    double totalSum = 0.0;
    std::vector<double> perCoreUsages;
    double selfCpu = 0.0;

#if __linux__
    perCoreUsages = readPerCoreUsages(totalSum);
    selfCpu = readSelfCPUUsage();
#else
    perCoreUsages = mockCoreUsages();
    totalSum = 80.0;
    selfCpu = mockSelfUsage();
#endif

    double selfShareVsTotal = (totalSum > 0.0) ? (selfCpu / totalSum) * 100.0 : 0.0;
    double selfShareVsSystemCap = (!perCoreUsages.empty())
                                      ? (selfCpu / (perCoreUsages.size() * 100.0)) * 100.0
                                      : 0.0;

    return CPUUsageSnapshot{
        perCoreUsages,
        totalSum,
        selfCpu,
        selfShareVsTotal,
        selfShareVsSystemCap
    };
}

#if __linux__
std::vector<double> CPUUsageMonitor::readPerCoreUsages(double &outSum) {
    std::ifstream statFile("/proc/stat");
    std::string line;
    std::vector<double> usages;
    outSum = 0.0;

    if (!statFile) {
        std::cerr << "[ERROR] Could not open /proc/stat\n";
        return {};
    }

    int cpuIndex = 0;
    while (std::getline(statFile, line)) {
        if (line.rfind("cpu", 0) == 0 && line.length() > 3 && std::isdigit(line[3])) {
            std::istringstream ss(line);
            std::string label;
            long long user, nice, system, idle, iowait, irq, softirq, steal;
            ss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            long long total = user + nice + system + idle + iowait + irq + softirq + steal;
            long long currIdle = idle + iowait;

            if (cpuIndex >= static_cast<int>(prevTotal_.size())) {
                prevTotal_.resize(cpuIndex + 1, 0);
                prevIdle_.resize(cpuIndex + 1, 0);
            }

            long long deltaTotal = total - prevTotal_[cpuIndex];
            long long deltaIdle = currIdle - prevIdle_[cpuIndex];
            long long deltaUsed = deltaTotal - deltaIdle;

            prevTotal_[cpuIndex] = total;
            prevIdle_[cpuIndex] = currIdle;

            double usage = (deltaTotal > 0)
                ? (static_cast<double>(deltaUsed) * 100.0 / deltaTotal)
                : 0.0;

            usages.push_back(usage);
            outSum += usage;
            ++cpuIndex;
        }
    }

    coreCount_ = usages.size();
    return usages;
}

double CPUUsageMonitor::readSelfCPUUsage() {
    std::ifstream selfFile("/proc/self/stat");
    std::string line;
    if (!std::getline(selfFile, line)) return 0.0;

    std::istringstream ss(line);
    std::string token;
    std::string ignore, comm;
    ss >> ignore >> comm;

    long utime = 0, stime = 0;

    for (int i = 3; i <= 15; ++i) {
        ss >> token;
        if (i == 14) utime = std::stol(token);
        if (i == 15) stime = std::stol(token);
    }

    long long currentJiffies = utime + stime;
    auto currentTime = std::chrono::steady_clock::now();

    if (prevSelfJiffies_ == -1) {
        prevSelfJiffies_ = currentJiffies;
        prevSelfTime_ = currentTime;
        return 0.0;
    }

    long long deltaJiffies = currentJiffies - prevSelfJiffies_;
    prevSelfJiffies_ = currentJiffies;

    std::chrono::duration<double> wallElapsed = currentTime - prevSelfTime_;
    prevSelfTime_ = currentTime;

    const double jiffiesPerSec = static_cast<double>(sysconf(_SC_CLK_TCK));
    if (wallElapsed.count() <= 0.0) return 0.0;

    return (static_cast<double>(deltaJiffies) * 100.0) / (jiffiesPerSec * wallElapsed.count());
}
#else
std::vector<double> CPUUsageMonitor::mockCoreUsages() const {
    return {20.0, 25.0, 15.0, 20.0};
}

double CPUUsageMonitor::mockSelfUsage() const {
    return 18.0;
}
#endif
