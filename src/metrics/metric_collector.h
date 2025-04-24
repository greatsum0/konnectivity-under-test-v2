#pragma once

#include <vector>
#include <mutex>
#include <chrono>

struct MetricSample {
    double timestampSec; // time passed since the start
    double bytesTransferred; // bytes in this interval
    double throughputMbps; // throughput in this interval
    double selfCpuUsage; // CPU usage of this process
    double totalCpuUsage; // CPU usage of all processes
};

class MetricCollector {
public:
    /**
     * @param metricNames  e.g. {"total_cpu_util","tool_cpu_util","throughput"}
     */
    explicit MetricCollector(const std::vector<std::string>& metricNames);

    // add bytes from traffic send/receive
    void addBytesTransferred(size_t bytes);

    // called by scheduler every interval
    void collect();

    // results
    const std::vector<MetricSample>& getSamples() const;

private:
    std::mutex mtx_;
    size_t bytesSinceLast_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastCollectTime_;
    std::vector<MetricSample> samples_;
    std::vector<std::string> metricNames_;

#ifdef __linux__
    int coreCount_;
    double readTotalCPUUsage() const;
    double readSelfCPUUsage() const;
#endif
};
