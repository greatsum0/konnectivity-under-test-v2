#include "total_cpu_collector.h"

TotalCpuCollector::TotalCpuCollector(std::vector<Metric>& store)
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
    _store.push_back({ts, "cpu_total_%", totalCpuPct});
}
