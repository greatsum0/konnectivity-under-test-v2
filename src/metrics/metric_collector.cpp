#include "metric_collector.h"

#ifdef __linux__
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#endif

MetricCollector::MetricCollector(const std::vector<std::string> &metricNames)
    : bytesSinceLast_(0),
      startTime_(std::chrono::steady_clock::now()),
      lastCollectTime_(startTime_),
      metricNames_(metricNames)
#ifdef __linux__
    , coreCount_(sysconf(_SC_NPROCESSORS_ONLN))
#endif
{
}

void MetricCollector::addBytesTransferred(size_t bytes) {
    std::lock_guard<std::mutex> lk(mtx_);
    bytesSinceLast_ += bytes;
}

const std::vector<MetricSample> &MetricCollector::getSamples() const {
    return samples_;
}

void MetricCollector::collect() {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    double interval = std::chrono::duration<double>(now - lastCollectTime_).count();

    size_t bytes; {
        std::lock_guard<std::mutex> lk(mtx_);
        bytes = bytesSinceLast_;
        bytesSinceLast_ = 0;
    }

    // Prepare sample with defaults
    MetricSample sample{};
    sample.timestampSec = elapsed;
    sample.bytesTransferred = bytes;
    sample.throughputMbps = 0.0;
    sample.selfCpuUsage = 0.0;
    sample.totalCpuUsage = 0.0;

    //Bytes transferred
    if (std::ranges::find(metricNames_, "bytes_transferred") != metricNames_.end()) {
        sample.bytesTransferred = bytes;
    }

    // Throughput
    if (std::ranges::find(metricNames_, "throughput") != metricNames_.end()) {
        const double mbits = bytes * 8.0 / 1e6;
        sample.throughputMbps = interval > 0 ? mbits / interval : 0.0;
    }

    // Per‐process CPU
    if (std::ranges::find(metricNames_, "tool_cpu_util") != metricNames_.end()) {
#ifdef __linux__
        sample.selfCpuUsage = readSelfCPUUsage();
#else
        sample.selfCpuUsage = 1.0;
#endif
    }

    // Total system CPU
    if (std::ranges::find(metricNames_, "total_cpu_util") != metricNames_.end()) {
#ifdef __linux__
        sample.totalCpuUsage = readTotalCPUUsage();
#else
        sample.totalCpuUsage = 2.0;
#endif
    }

    samples_.push_back(sample);
    lastCollectTime_ = now;
}

#ifdef __linux__

double MetricCollector::readTotalCPUUsage() const {
    std::ifstream statFile("/proc/stat");
    std::string line;
    std::getline(statFile, line);
    std::istringstream ss(line);

    std::string cpuLabel;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long long totalTime = user + nice + system + idle + iowait + irq + softirq + steal;
    long long idleTime = idle;

    static long long previousTotal = 0, previousIdle = 0;
    long long deltaTotal = totalTime - previousTotal;
    long long deltaIdle = idleTime - previousIdle;
    long long deltaUsed = deltaTotal - deltaIdle;

    previousTotal = totalTime;
    previousIdle = idleTime;

    return (deltaTotal > 0)
           ? (static_cast<double>(deltaUsed) * 100.0 / static_cast<double>(deltaTotal))
           : 0.0;
}


/**
 * Reads and calculates this process's CPU usage percentage over wall time, across all cores
 */
double MetricCollector::readSelfCPUUsage() const {
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

    static long previousProcessJiffies = 0;
    long currentProcessJiffies = utime + stime;
    long deltaProcess = currentProcessJiffies - previousProcessJiffies;
    previousProcessJiffies = currentProcessJiffies;

    static auto previousTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> wallElapsed = currentTime - previousTime;
    previousTime = currentTime;

    const double jiffiesPerSec = static_cast<double>(sysconf(_SC_CLK_TCK));
    return (wallElapsed.count() > 0)
           ? (static_cast<double>(deltaProcess) * 100.0 * coreCount_
              / (jiffiesPerSec * wallElapsed.count()))
           : 0.0;
}

#endif
