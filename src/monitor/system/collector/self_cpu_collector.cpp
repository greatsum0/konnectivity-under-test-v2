#include "self_cpu_collector.h"

SelfCpuCollector::SelfCpuCollector(std::vector<Metric> &store)
    : _store(store) {
}

void SelfCpuCollector::onCpuSample(
    std::chrono::system_clock::time_point ts,
    double,
    double selfCpuPct
) {
    _store.push_back({ts, "cpu_self_%", selfCpuPct});
}
