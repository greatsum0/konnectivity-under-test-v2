#include "network_collector.h"

NetworkCollector::NetworkCollector(std::vector<Metric> &store)
    : _store(store) {
}

void NetworkCollector::onNetworkSample(std::chrono::system_clock::time_point ts,
                                       uint64_t bytesIn, uint64_t bytesOut,
                                       uint64_t packetsIn, uint64_t packetsOut,
                                       double throughput) {
    _store.push_back({ts, "bytes_in", static_cast<double>(bytesIn)});
    _store.push_back({ts, "bytes_out", static_cast<double>(bytesOut)});
    _store.push_back({ts, "packets_in", static_cast<double>(packetsIn)});
    _store.push_back({ts, "packets_out", static_cast<double>(packetsOut)});
    _store.push_back({ts, "throughput_Mbps", throughput});
}
