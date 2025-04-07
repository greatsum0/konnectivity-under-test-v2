#include "metric_collector.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>

MetricCollector::MetricCollector(asio::io_context &io_context,
                                 double interval_sec,
                                 const std::vector<std::string> &enabledMetrics,
                                 std::string outputCsvPath)
    : timer_(io_context),
      interval_sec_(interval_sec),
      bytesTransferred_(0),
      elapsed_sec_(0.0),
      enabledMetrics_(enabledMetrics),
      outputCsvPath_(std::move(outputCsvPath)),
      dataManager_(enabledMetrics),
      coreCount_(sysconf(_SC_NPROCESSORS_ONLN)) {
}

void MetricCollector::start() {
    measure();
}

void MetricCollector::start(std::chrono::steady_clock::time_point referenceTime) {
    lastWallTime_ = referenceTime;
    elapsed_sec_ = 0.0;
    measure();
}

void MetricCollector::stop() {
    dataManager_.printTable();

    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d-%H-%M-%S");

    dataManager_.saveAsCSV(outputCsvPath_ + ss.str() + ".csv");
    reset();
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

/**
 * Reads and calculates the total CPU usage percentage over all cores
 */
double MetricCollector::readTotalCPUUsage() const {
    static long long previousTotal = 0, previousIdle = 0;

    std::ifstream statFile("/proc/stat");
    std::string line;
    std::getline(statFile, line);
    std::istringstream ss(line);

    std::string cpuLabel;
    long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long long totalTime = user + nice + system + idle + iowait + irq + softirq + steal;
    long long idleTime = idle;

    long long deltaTotal = totalTime - previousTotal;
    long long deltaIdle = idleTime - previousIdle;
    long long deltaUsed = deltaTotal - deltaIdle;

    previousTotal = totalTime;
    previousIdle = idleTime;

    return (deltaTotal > 0)
        ? (static_cast<double>(deltaUsed) * 100.0 * coreCount_ / static_cast<double>(deltaTotal))
        : 0.0;
}

/**
 * Reads and calculates this process's CPU usage percentage over wall time, across all cores
 */
double MetricCollector::readSelfCPUUsage() const {
    static long previousProcessJiffies = 0;
    static const auto jiffiesPerSec = static_cast<double>(sysconf(_SC_CLK_TCK));
    static auto previousTime = std::chrono::steady_clock::now();

    std::ifstream selfFile("/proc/self/stat");
    std::string line;
    std::getline(selfFile, line);
    std::istringstream selfStream(line);

    std::string token;
    long utime = 0, stime = 0;
    for (int i = 1; i <= 15; ++i) {
        selfStream >> token;
        if (i == 14) utime = std::stol(token); // user time
        if (i == 15) stime = std::stol(token); // system time
    }

    long currentProcessJiffies = utime + stime;
    long deltaProcess = currentProcessJiffies - previousProcessJiffies;
    previousProcessJiffies = currentProcessJiffies;

    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> wallElapsed = currentTime - previousTime;
    previousTime = currentTime;

    return (wallElapsed.count() > 0)
        ? (static_cast<double>(deltaProcess) * 100.0 * coreCount_ / (jiffiesPerSec * wallElapsed.count()))
        : 0.0;
}

void MetricCollector::measure() {
    timer_.expires_after(std::chrono::milliseconds(static_cast<int>(interval_sec_ * 1000)));
    timer_.async_wait([this](const asio::error_code &ec) {
        if (!ec) {
            if (isMetricEnabled("total_cpu_util")) {
                double totalCPU = readTotalCPUUsage();
                dataManager_.addMetricValue("total_cpu_util", elapsed_sec_, totalCPU);
            }

            if (isMetricEnabled("tool_cpu_util")) {
                double selfCPU = readSelfCPUUsage();
                dataManager_.addMetricValue("tool_cpu_util", elapsed_sec_, selfCPU);
            }

            if (isMetricEnabled("throughput")) {
                double throughputMbps = (bytesTransferred_ * 8.0) / (interval_sec_ * 1'000'000);
                dataManager_.addMetricValue("throughput", elapsed_sec_, throughputMbps);
            }

            elapsed_sec_ += interval_sec_;
            bytesTransferred_ = 0;
            measure();
        }
    });
}

void MetricCollector::reset() {
    timer_.cancel();
    bytesTransferred_ = 0;
    elapsed_sec_ = 0.0;
    dataManager_.clear();
    lastWallTime_ = std::chrono::steady_clock::now();
}