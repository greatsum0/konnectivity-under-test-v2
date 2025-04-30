#include "self_cpu_collector.h"

SelfCpuCollector::SelfCpuCollector(std::vector<Metric> &store)
    : _store(store) {
}

void SelfCpuCollector::onNetworkSample(
    std::chrono::system_clock::time_point,
    uint64_t,
    uint64_t) {
    // no‐op
}

void SelfCpuCollector::onCpuSample(
    std::chrono::system_clock::time_point ts,
    double,
    double selfCpuPct
) {
    _store.push_back({ts, "cpu_self_%", selfCpuPct});
}
