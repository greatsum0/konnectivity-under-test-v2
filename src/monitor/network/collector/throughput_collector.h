#pragma once

#include "metric.h"
#include <vector>

#include "imetric_collector.h"

/// Converts bytes_in/out → mbps_in/out using the interval.
class ThroughputCollector final : public IMetricCollector {
public:
    ThroughputCollector(std::vector<Metric> &store, double intervalSec);

    void onNetworkSample(
        std::chrono::system_clock::time_point ts,
        uint64_t bytesIn,
        uint64_t bytesOut,
        uint64_t packetsIn,
        uint64_t packetsOut
    ) override;

    void onCpuSample(
        std::chrono::system_clock::time_point,
        double,
        double) override;

private:
    std::vector<Metric> &_store;
    double _interval;
};
