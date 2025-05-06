#pragma once

#include "metric_store.h"
#include <vector>
#include "imetric_collector.h"

/// Records total and per-core CPU % metrics into the store.
class TotalCpuCollector final : public IMetricCollector {
public:
    explicit TotalCpuCollector(std::vector<Metric> &store);

    void onNetworkSample(
        std::chrono::system_clock::time_point,
        uint64_t,
        uint64_t) override;

    void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double totalCpuPct,
        double) override;

    void onCpuDetailSample(
        std::chrono::system_clock::time_point ts,
        double cpu0,
        double cpu1,
        double cpu2,
        double cpu3,
        double selfShareTotal,
        double selfShareCap) override;

private:
    std::vector<Metric> &_store;
};
