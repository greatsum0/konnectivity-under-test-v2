#include "throughput_collector.h"

ThroughputCollector::ThroughputCollector(std::vector<Metric> &store, const double intervalSec)
    : _store(store), _interval(intervalSec) {
}

void ThroughputCollector::onNetworkSample(
    const std::chrono::system_clock::time_point ts,
    const uint64_t bytesIn,
    const uint64_t bytesOut
) {
    _store.push_back({ts, "throughput_in_Mbps", (bytesIn * 8.0) / (1e6 * _interval)});
    _store.push_back({ts, "throughput_out_Mbps", (bytesOut * 8.0) / (1e6 * _interval)});
}

void ThroughputCollector::onCpuSample(
    std::chrono::system_clock::time_point,
    double,
    double) {
    // no‐op
}
