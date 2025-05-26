#pragma once
#include <metric.h>

#include "imetric_collector.h"

class NetworkCollector : public IMetricCollector {
public:
    explicit NetworkCollector(std::vector<Metric> &store);

    void onNetworkSample(std::chrono::system_clock::time_point ts,
                         uint64_t bytesIn, uint64_t bytesOut,
                         uint64_t packetsIn, uint64_t packetsOut, uint64_t throughput) override;

    void onCpuSample(std::chrono::system_clock::time_point, double, double) override {
    }

private:
    std::vector<Metric> &_store;
};
