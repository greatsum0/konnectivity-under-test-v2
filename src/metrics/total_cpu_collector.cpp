#include "total_cpu_collector.h"

TotalCpuCollector::TotalCpuCollector(std::vector<Metric> &store)
  : _store(store)
{}

void TotalCpuCollector::onNetworkSample(
    std::chrono::system_clock::time_point,
    uint64_t,
    uint64_t
) {
    // no‐op
}

void TotalCpuCollector::onCpuSample(
    const std::chrono::system_clock::time_point ts,
    const double totalCpuPct,
    double
) {
    _store.push_back({ts, "sum_cores_%", totalCpuPct});
}

void TotalCpuCollector::onCpuDetailSample(
    std::chrono::system_clock::time_point ts,
    const std::vector<double> &perCoreUsages,
    double selfShareTotal,
    double selfShareCap
) {
    for (size_t i = 0; i < perCoreUsages.size(); ++i) {
        _store.push_back({ts, "cpu" + std::to_string(i) + "_%", perCoreUsages[i]});
    }
    _store.push_back({ts, "self_share_/_total_load_%", selfShareTotal});
    _store.push_back({ts, "self_share_/_system_cap_%", selfShareCap});
}

