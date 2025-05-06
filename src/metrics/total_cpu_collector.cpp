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
    const double cpu0,
    const double cpu1,
    const double cpu2,
    const double cpu3,
    const double selfShareTotal,
    const double selfShareCap
) {
    _store.push_back({ts, "cpu0_%", cpu0});
    _store.push_back({ts, "cpu1_%", cpu1});
    _store.push_back({ts, "cpu2_%", cpu2});
    _store.push_back({ts, "cpu3_%", cpu3});
    _store.push_back({ts, "self_share_/_total_load_%", selfShareTotal});
    _store.push_back({ts, "self_share_/_system_cap_%", selfShareCap});
}
