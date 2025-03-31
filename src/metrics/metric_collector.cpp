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
                                 std::string outputCsvPath)
    : timer_(io_context),
      interval_sec_(interval_sec),
      bytesTransferred_(0),
      elapsed_sec_(0.0),
      enabledMetrics_(enabledMetrics),
      outputCsvPath_(std::move(outputCsvPath)),
      dataManager_(enabledMetrics),
      coreCount_(sysconf(_SC_NPROCESSORS_ONLN)) {}

void MetricCollector::start() {
    measure();
}

void MetricCollector::start(std::chrono::steady_clock::time_point referenceTime) {
    lastWallTime_ = referenceTime;  // ölçüm döngüsü bu zamanla senkronize başlasın
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

bool MetricCollector::isMetricEnabled(const std::string& name) const {
    return std::ranges::find(enabledMetrics_, name) != enabledMetrics_.end();
}

double MetricCollector::getTotalCPUUsage() {
#ifdef __APPLE__
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                                       reinterpret_cast<host_info_t>(&cpuinfo), &count);
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

static long lastProcessJiffies = 0;
static uint64_t lastCgroupUsageMicros = 0;
//static auto lastWallTime = std::chrono::steady_clock::now();

double MetricCollector::getNormalizedProcessCPUUsage() {
#ifdef __APPLE__
    task_thread_times_info_data_t info;
    mach_msg_type_number_t count = TASK_THREAD_TIMES_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
        return 0.0;

    static double lastTotalSec = 0.0;
    auto now = std::chrono::steady_clock::now();
    double wallElapsedSec = std::chrono::duration<double>(now - lastWallTime_).count();
    lastWallTime_ = now;

    double currentTotalSec = info.user_time.seconds + info.system_time.seconds +
                             info.user_time.microseconds / 1e6 + info.system_time.microseconds / 1e6;

    double delta = currentTotalSec - lastTotalSec;
    lastTotalSec = currentTotalSec;

    if (delta <= 0.0 || wallElapsedSec <= 0.0 || coreCount_ <= 0) return 0.0;

    return (delta / wallElapsedSec) * 100.0 / coreCount_;
#else
    std::ifstream statFile("/proc/self/stat");
    if (!statFile.is_open()) return 0.0;

    std::string line;
    std::getline(statFile, line);
    statFile.close();

    std::istringstream ss(line);
    std::string token;
    long utime = 0, stime = 0;
    for (int i = 1; i <= 15; ++i) {
        ss >> token;
        if (i == 14) utime = std::stol(token);
        if (i == 15) stime = std::stol(token);
    }
    long currentProcessJiffies = utime + stime;

    std::vector<std::string> cgroupPaths = {
        "/sys/fs/cgroup/cpu.stat",
        "/sys/fs/cgroup/unified/cpu.stat"
    };

    uint64_t usage_usec = 0;
    for (const auto& path : cgroupPaths) {
        std::ifstream cgroupFile(path);
        if (!cgroupFile.is_open()) continue;

        std::string line;
        while (std::getline(cgroupFile, line)) {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            if (iss >> key >> value && key == "usage_usec") {
                usage_usec = value;
                break;
            }
        }
        if (usage_usec > 0) break;
    }

    auto currentTime = std::chrono::steady_clock::now();
    double wallElapsedSec = std::chrono::duration<double>(currentTime - lastWallTime).count();
    lastWallTime = currentTime;

    long deltaProcess = currentProcessJiffies - lastProcessJiffies;
    lastProcessJiffies = currentProcessJiffies;

    uint64_t deltaCgroup = usage_usec - lastCgroupUsageMicros;
    lastCgroupUsageMicros = usage_usec;

    if (deltaProcess == 0 || deltaCgroup == 0) return 0.0;

    if (coreCount_ <= 0 || wallElapsedSec <= 0.0) {
        std::cerr << "[WARN] Invalid core count or wall time. Skipping CPU usage calculation." << std::endl;
        return 0.0;
    }

    long ticksPerSec = sysconf(_SC_CLK_TCK);
    double processTimeSec = static_cast<double>(deltaProcess) / ticksPerSec;

    return (processTimeSec / wallElapsedSec) * 100.0 / coreCount_;
#endif
}

double MetricCollector::getProcessCPUUsage() const {
    return 0.0; // Deprecated
}

void MetricCollector::measure() {
    auto startTime = std::chrono::steady_clock::now();

    timer_.expires_after(std::chrono::milliseconds(static_cast<int>(interval_sec_ * 1000)));
    timer_.async_wait([this, startTime](const asio::error_code& ec) {
        if (!ec) {
            auto endTime = std::chrono::steady_clock::now();
            double actualIntervalSec = std::chrono::duration<double>(endTime - startTime).count();
            elapsed_sec_ += actualIntervalSec;

            if (isMetricEnabled("total_cpu_util")) {
                double totalCPU = getTotalCPUUsage();
                dataManager_.addMetricValue("total_cpu_util", elapsed_sec_, totalCPU);
            }

            if (isMetricEnabled("tool_cpu_util")) {
                double normalized = getNormalizedProcessCPUUsage();
                dataManager_.addMetricValue("tool_cpu_util", elapsed_sec_, normalized * coreCount_);
                dataManager_.addMetricValue("tool_cpu_util_norm", elapsed_sec_, normalized);
            }

            if (isMetricEnabled("throughput")) {
                double throughput_mbps = (bytesTransferred_ * 8.0) / (actualIntervalSec * 1'000'000);
                dataManager_.addMetricValue("throughput", elapsed_sec_, throughput_mbps);
            }

            bytesTransferred_ = 0;
            measure();  // recursive çağrı ile devam
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
