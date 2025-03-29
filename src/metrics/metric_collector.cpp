#include "metric_collector.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <utility>
#include <ranges>
#include <algorithm>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#endif

MetricCollector::MetricCollector(asio::io_context& io_context,
                                 double interval_sec,
                                 const std::vector<std::string>& enabledMetrics,
                                 std::string  outputCsvPath)
    : timer_(io_context),
      interval_sec_(interval_sec),
      bytesTransferred_(0),
      elapsed_sec_(0.0),
      enabledMetrics_(enabledMetrics),
      outputCsvPath_(std::move(outputCsvPath)),
      dataManager_(enabledMetrics) {}


void MetricCollector::start() {
    measure();
}

void MetricCollector::stop() {
    timer_.cancel();
    dataManager_.printTable();
    dataManager_.saveAsCSV(outputCsvPath_);
}

void MetricCollector::addBytesTransferred(int bytes) {
    bytesTransferred_ += bytes;
}

std::shared_ptr<MetricDataManager> MetricCollector::getManager() const {
    return std::make_shared<MetricDataManager>(dataManager_);
}

bool MetricCollector::isMetricEnabled(const std::string &name) const {
    return std::ranges::find(enabledMetrics_, name) != enabledMetrics_.end();
}

double MetricCollector::getTotalCPUUsage() {
#ifdef __APPLE__
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, reinterpret_cast<host_info_t>(&cpuinfo),
                                       &count);
    if (kr != KERN_SUCCESS) return 0.0;

    static uint64_t prevTotalTicks = 0;
    static uint64_t prevIdleTicks = 0;

    const uint64_t user = cpuinfo.cpu_ticks[CPU_STATE_USER];
    const uint64_t system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
    const uint64_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
    const uint64_t nice = cpuinfo.cpu_ticks[CPU_STATE_NICE];

    const uint64_t totalTicks = user + system + idle + nice;
    const uint64_t totalDiff = totalTicks - prevTotalTicks;
    const uint64_t idleDiff = idle - prevIdleTicks;

    prevTotalTicks = totalTicks;
    prevIdleTicks = idle;

    if (totalDiff == 0) return 0.0;
    return (100.0 * (totalDiff - idleDiff)) / totalDiff;
#else
    static long long lastIdle = 0, lastTotal = 0;

    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) return 0.0;

    std::string line;
    std::getline(statFile, line);
    std::istringstream ss(line);

    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long long idleTime = idle + iowait;
    long long totalTime = user + nice + system + idle + iowait + irq + softirq + steal;

    long long deltaIdle = idleTime - lastIdle;
    long long deltaTotal = totalTime - lastTotal;

    lastIdle = idleTime;
    lastTotal = totalTime;

    if (deltaTotal == 0) return 0.0;
    return 100.0 * (deltaTotal - deltaIdle) / deltaTotal;
#endif
}

// function that normalizes process CPU usage
double MetricCollector::getNormalizedProcessCPUUsage() const {
    double toolCPU = getProcessCPUUsage();
    int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    if(numCores > 0)
        return toolCPU / numCores;
    return toolCPU;
}

double MetricCollector::getProcessCPUUsage() const {
    struct rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    const double userTime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    const double sysTime = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    const double totalTime = userTime + sysTime;

    static double lastTotalTime = 0;
    static double lastElapsedTime = 0;

    const double currentTime = totalTime;
    const double currentWall = elapsed_sec_;

    const double deltaTime = currentTime - lastTotalTime;
    const double deltaWall = currentWall - lastElapsedTime;

    lastTotalTime = currentTime;
    lastElapsedTime = currentWall;

    if (deltaWall == 0) return 0.0;
    return (deltaTime / deltaWall) * 100.0;
}

void MetricCollector::measure() {
    timer_.expires_after(std::chrono::milliseconds(static_cast<int>(interval_sec_ * 1000)));
    timer_.async_wait([this](const asio::error_code& ec) {
        if (!ec) {
            elapsed_sec_ += interval_sec_;

            if (isMetricEnabled("total_cpu_util")) {
                double totalCPU = getTotalCPUUsage();
                dataManager_.addMetricValue("total_cpu_util", elapsed_sec_, totalCPU);
            }

            if (isMetricEnabled("tool_cpu_util")) {
                double procCPU = getProcessCPUUsage();
                int numCores = sysconf(_SC_NPROCESSORS_ONLN);
                double normalizedProcCPU = (numCores > 0) ? procCPU / numCores : procCPU;
                dataManager_.addMetricValue("tool_cpu_util", elapsed_sec_, procCPU);
                dataManager_.addMetricValue("tool_cpu_util_norm", elapsed_sec_, normalizedProcCPU);
            }

            if (isMetricEnabled("throughput")) {
                double throughput_mbps = (bytesTransferred_ * 8.0) / (interval_sec_ * 1'000'000);
                dataManager_.addMetricValue("throughput", elapsed_sec_, throughput_mbps);
            }

            bytesTransferred_ = 0;
            measure(); // recursive
        }
    });
}

void MetricCollector::reset() {
    bytesTransferred_ = 0;
    elapsed_sec_ = 0.0;
    dataManager_.clear();
}
