#include "system_collector.h"

SystemCollector::SystemCollector(std::vector<Metric> &store)
    : _store(store) {
}

void SystemCollector::onCpuSample(
    std::chrono::system_clock::time_point ts,
    double totalCpu,
    double selfCpu
) {
    lastTotalCpu_ = totalCpu;
    lastSelfCpu_ = selfCpu;

    _store.push_back({ts, "total_cpu_util", totalCpu});
    _store.push_back({ts, "tool_cpu_util", selfCpu});
}

void SystemCollector::onCpuDetailSample(
    std::chrono::system_clock::time_point ts,
    const std::vector<double> &perCore,
    double selfShareVsTotal,
    double selfShareVsSystemCap
) {
    _store.push_back({ts, "self_share_/_total_load_%", selfShareVsTotal});
    _store.push_back({ts, "self_share_/_system_cap_%", selfShareVsSystemCap});
    _store.push_back({ts, "sum_cores_%", lastTotalCpu_});
    _store.push_back({ts, "cpu_self_%", lastSelfCpu_});

    for (size_t i = 0; i < perCore.size(); ++i) {
        const std::string label = "cpu" + std::to_string(i) + "_%";
        _store.push_back({ts, label, perCore[i]});
    }
}
