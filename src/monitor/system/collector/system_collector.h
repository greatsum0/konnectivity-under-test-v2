#pragma once

#include <metric.h>

#include "imetric_collector.h"
#include "metric_types.h"
#include <vector>

class SystemCollector : public IMetricCollector {
public:
    explicit SystemCollector(std::vector<Metric> &store);

    void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double totalCpu,
        double selfCpu
    ) override;

    void onCpuDetailSample(
        std::chrono::system_clock::time_point ts,
        const std::vector<double> &perCore,
        double selfShareVsTotal,
        double selfShareVsSystemCap
    ) override;

private:
    std::vector<Metric> &_store;
    double lastTotalCpu_{0.0};
    double lastSelfCpu_{0.0};
};
