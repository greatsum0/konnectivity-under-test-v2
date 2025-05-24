#pragma once

#include "metric.h"
#include <vector>

#include "imetric_collector.h"

/// Records only the process’s own CPU % into the store.
class SelfCpuCollector final : public IMetricCollector {
public:
    explicit SelfCpuCollector(std::vector<Metric> &store);

    void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double,
        double selfCpuPct
    ) override;

private:
    std::vector<Metric> &_store;
};
