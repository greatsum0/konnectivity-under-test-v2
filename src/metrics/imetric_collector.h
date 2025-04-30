#pragma once
#include <chrono>

struct IMetricCollector {
    virtual ~IMetricCollector() = default;

    // called when bytesIn/bytesOut are sampled
    virtual void onNetworkSample(
        std::chrono::system_clock::time_point ts,
        uint64_t bytesIn,
        uint64_t bytesOut
    ) = 0;

    // called when CPU samples arrive
    virtual void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double totalCpuPct,
        double selfCpuPct
    ) = 0;
};
