#pragma once

#include <string>
#include <functional>
#include <memory>

struct IMetricCollector {
    virtual ~IMetricCollector() = default;

    // called when bytesIn/bytesOut are sampled
    virtual void onNetworkSample(
        std::chrono::system_clock::time_point ts,
        uint64_t bytesIn,
        uint64_t bytesOut,
        uint64_t packetsIn,
        uint64_t packetsOut
    ) {
        // default no-op
    }

    // called when CPU samples arrive
    virtual void onCpuSample(
        std::chrono::system_clock::time_point ts,
        double totalCpuPct,
        double selfCpuPct
    ) {
        // default no-op
    }

    virtual void onCpuDetailSample(
        std::chrono::system_clock::time_point,
        const std::vector<double> &,
        double,
        double) {
        // default no-op
    }
};
