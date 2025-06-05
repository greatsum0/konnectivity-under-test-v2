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

    _store.push_back({ts, "cpu.util.host", totalCpu});
    _store.push_back({ts, "cpu.util.self", selfCpu});
}

void SystemCollector::onCpuDetailSample(
    std::chrono::system_clock::time_point ts,
    const std::vector<double> &perCore,
    double selfShareVsTotal,
    double selfShareVsSystemCap
) {
    _store.push_back({ts, "cpu.share.self_vs_host_load.perc", selfShareVsTotal});
    _store.push_back({ts, "cpu.share.self_vs_system_capacity.perc", selfShareVsSystemCap});
    _store.push_back({ts, "cpu.util.cores.sum.perc", lastTotalCpu_});
    _store.push_back({ts, "cpu.util.self.perc", lastSelfCpu_});

    for (size_t i = 0; i < perCore.size(); ++i) {
        const std::string label = "cpu.util.core." + std::to_string(i) + ".perc";
        _store.push_back({ts, label, perCore[i]});
    }
}
