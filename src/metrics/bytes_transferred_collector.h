#pragma once

#include "metric_store.h"
#include <vector>

#include "imetric_collector.h"

/// Collects bytes_in and bytes_out into the shared store.
class BytesTransferredCollector final : public IMetricCollector {
public:
    explicit BytesTransferredCollector(std::vector<Metric> &store);

    void onNetworkSample(
        std::chrono::system_clock::time_point ts,
        uint64_t bytesIn,
        uint64_t bytesOut,
        uint64_t packetsIn,
        uint64_t packetsOut
    ) override;

    void onCpuSample(
        std::chrono::system_clock::time_point,
        double,
        double) override;

private:
    std::vector<Metric> &_store;
};
