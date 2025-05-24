#include "throughput_collector.h"

ThroughputCollector::ThroughputCollector(std::vector<Metric> &store, const double intervalSec)
    : _store(store), _interval(intervalSec) {
}

void ThroughputCollector::onNetworkSample(
    const std::chrono::system_clock::time_point ts,
    const uint64_t bytesIn,
    const uint64_t bytesOut,
    const uint64_t packetsIn,
    const uint64_t packetsOut
) {
    _store.push_back({ts, "throughput_in_Mbps", (bytesIn * 8.0) / (1e6 * _interval)});
    _store.push_back({ts, "throughput_out_Mbps", (bytesOut * 8.0) / (1e6 * _interval)});
    _store.push_back({ts, "packets_in", static_cast<double>(packetsIn)});
    _store.push_back({ts, "packets_out", static_cast<double>(packetsOut)});
}

void ThroughputCollector::onCpuSample(
    std::chrono::system_clock::time_point,
    double,
    double) {
    // no‐op
}
