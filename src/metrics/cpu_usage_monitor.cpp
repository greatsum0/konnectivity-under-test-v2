#include "cpu_usage_monitor.h"

#include <fstream>
#include <sstream>
#include <string>

CPUUsageMonitor::CPUUsageMonitor(const double intervalSec)
    : intervalSec_(intervalSec)
#if __linux__
    , coreCount_(sysconf(_SC_NPROCESSORS_ONLN))
#endif
{
#if __linux__
    prevTotal_.resize(coreCount_, 0);
    prevIdle_.resize(coreCount_, 0);
#endif
}

CPUUsageMonitor::~CPUUsageMonitor() {
    stop();
}

void CPUUsageMonitor::start(ExtendedCallback cb) {
    if (running_) return;
    callback_ = std::move(cb);
    running_ = true;

    timerThread_ = std::thread([this]() {
        auto next = std::chrono::steady_clock::now() + std::chrono::duration<double>(intervalSec_);
        while (running_) {
            std::this_thread::sleep_until(next);
            next += std::chrono::duration<double>(intervalSec_);

            double totalSum = 0.0;
            std::vector<double> cores = readPerCoreUsages(totalSum);
            const double selfCpu = readSelfCPUUsage();

            const double selfShareVsTotal = (totalSum > 0.0) ? (selfCpu / totalSum) * 100.0 : 0.0;
            const double selfShareVsSystemCap = (cores.size() > 0)
                ? (selfCpu / (cores.size() * 100.0)) * 100.0
                : 0.0;

            if (callback_)
                callback_(
                    cores,
                    totalSum,
                    selfCpu,
                    selfShareVsTotal,
                    selfShareVsSystemCap
                );
        }
    });
}

void CPUUsageMonitor::stop() {
    if (!running_) return;
    running_ = false;
    if (timerThread_.joinable()) timerThread_.join();
}

std::vector<double> CPUUsageMonitor::readPerCoreUsages(double &outSum) const {
#if __linux__
    std::ifstream statFile("/proc/stat");
    std::string line;
    std::vector<double> usages;
    outSum = 0.0;

    int cpuIndex = 0;
    while (std::getline(statFile, line)) {
        if (line.rfind("cpu", 0) == 0 && line.length() > 3 && std::isdigit(line[3])) {
            std::istringstream ss(line);
            std::string label;
            long long user, nice, system, idle, iowait, irq, softirq, steal;
            ss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            long long total = user + nice + system + idle + iowait + irq + softirq + steal;
            long long currIdle = idle + iowait;

            if (cpuIndex >= prevTotal_.size()) {
                prevTotal_.resize(cpuIndex + 1, 0);
                prevIdle_.resize(cpuIndex + 1, 0);
            }

            if (prevTotal_[cpuIndex] == 0 && prevIdle_[cpuIndex] == 0) {
                prevTotal_[cpuIndex] = total;
                prevIdle_[cpuIndex] = currIdle;
                usages.push_back(0.0);
            } else {
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
            }

            ++cpuIndex;
        }
    }

    return usages;

#else
    // MOCK VALUES for non-Linux
    outSum = 80.0;
    return {20.0, 25.0, 15.0, 20.0};
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
    for (int i = 1; i <= 15; ++i) {
        selfStream >> token;
        if (i == 14) utime = std::stol(token);
        if (i == 15) stime = std::stol(token);
    }

    long long currentJiffies = utime + stime;
    static long long previousJiffies = -1;
    static auto previousTime = std::chrono::steady_clock::now();

    auto currentTime = std::chrono::steady_clock::now();

    if (previousJiffies == -1) {
        previousJiffies = currentJiffies;
        previousTime = currentTime;
        return 0.0;
    }

    long long deltaJiffies = currentJiffies - previousJiffies;
    previousJiffies = currentJiffies;

    std::chrono::duration<double> wallElapsed = currentTime - previousTime;
    previousTime = currentTime;

    const double jiffiesPerSec = static_cast<double>(sysconf(_SC_CLK_TCK));
    if (wallElapsed.count() <= 0.0) return 0.0;

    return (static_cast<double>(deltaJiffies) * 100.0)
           / (jiffiesPerSec * wallElapsed.count());
#else
    return 18.0; // MOCK: %18 self CPU
#endif
}
