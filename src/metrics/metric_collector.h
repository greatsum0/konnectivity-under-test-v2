#pragma once

#include <vector>
#include <mutex>
#include <chrono>

struct MetricSample {
    double timestampSec; // time passed since the start
    double bytesTransferred; // bytes in this interval
    double throughputMbps; // throughput in this interval
    double cpuUsagePercent;
};

class MetricCollector {
public:
    MetricCollector();

    // counts the sent bytes since the last collection
    void addBytesTransferred(size_t bytes);

    // called every interval
    void collect();

    // returns the last collected sample
    [[nodiscard]] const std::vector<MetricSample> &getSamples() const;

private:
    std::mutex mtx_;
    size_t bytesSinceLast_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastCollectTime_;
    std::vector<MetricSample> samples_;

    double sampleCpuUsage();
};
