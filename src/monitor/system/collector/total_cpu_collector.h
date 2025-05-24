#pragma once

#include "metric.h"
#include <vector>
#include "imetric_collector.h"

/// Records total and per-core CPU % metrics into the store.
class TotalCpuCollector final : public IMetricCollector {
public:
    explicit TotalCpuCollector(std::vector<Metric> &store);

    void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double totalCpuPct,
        double) override;

    void onCpuDetailSample(
        std::chrono::system_clock::time_point ts,
        const std::vector<double> &perCoreUsages,
        double selfShareTotal,
        double selfShareCap) override;

private:
    std::vector<Metric> &_store;
};
