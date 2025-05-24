#include "cpu_usage_monitor.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

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

CPUUsageMonitor::~CPUUsageMonitor() {
    stop();
}

void CPUUsageMonitor::start(ExtendedCallback cb) {
    if (running_) {
        std::cerr << "[WARN] CPUUsageMonitor already running.\n";
        return;
    }

    callback_ = std::move(cb);
    running_ = true;

    timerThread_ = std::thread([this]() {
        std::unique_lock<std::mutex> lock(mtx_);
        while (running_) {
            auto nextWake = std::chrono::steady_clock::now() + std::chrono::duration<double>(intervalSec_);
            cv_.wait_until(lock, nextWake, [this] { return !running_; });

            if (!running_) break;

            double totalSum = 0.0;
            std::vector<double> perCoreUsages = readPerCoreUsages(totalSum);
            double selfCpu = readSelfCPUUsage();

            double selfShareVsTotal = (totalSum > 0.0) ? (selfCpu / totalSum) * 100.0 : 0.0;
            double selfShareVsSystemCap = (perCoreUsages.size() > 0)
                                              ? (selfCpu / (perCoreUsages.size() * 100.0)) * 100.0
                                              : 0.0;

            if (callback_) {
                CPUUsageSnapshot snapshot{perCoreUsages, totalSum, selfCpu, selfShareVsTotal, selfShareVsSystemCap};
                callback_(snapshot);
            }
        }
    });
}

void CPUUsageMonitor::stop() {
    if (!running_) return;
    running_ = false;
    cv_.notify_all();
    if (timerThread_.joinable()) timerThread_.join();
}

std::vector<double> CPUUsageMonitor::readPerCoreUsages(double &outSum) {
#if __linux__
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
#else
    outSum = 80.0;
    return {20.0, 25.0, 15.0, 20.0};
#endif
}

double CPUUsageMonitor::readSelfCPUUsage() {
#if __linux__
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
#else
    return 18.0; // MOCK
#endif
}

CPUUsageSnapshot CPUUsageMonitor::sample() {
    double totalSum = 0.0;
    std::vector<double> perCoreUsages = readPerCoreUsages(totalSum);
    double selfCpu = readSelfCPUUsage();

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
