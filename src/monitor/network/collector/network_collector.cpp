#include "network_collector.h"

NetworkCollector::NetworkCollector(std::vector<Metric> &store)
    : _store(store) {
}

void NetworkCollector::onNetworkSample(std::chrono::system_clock::time_point ts,
                                       const uint64_t bytesIn,
                                       const uint64_t bytesOut,
                                       const uint64_t packetsIn,
                                       const uint64_t packetsOut,
                                       const double throughput,
                                       const double latency,
                                       const double jitter) {
    _store.push_back({ts, "net.bytes.in", static_cast<double>(bytesIn)});
    _store.push_back({ts, "net.bytes.out", static_cast<double>(bytesOut)});
    _store.push_back({ts, "net.packets.in", static_cast<double>(packetsIn)});
    _store.push_back({ts, "net.packets.out", static_cast<double>(packetsOut)});
    _store.push_back({ts, "net.throughput.mbps", throughput});
    _store.push_back({ts, "net.latency.us", latency});
    _store.push_back({ts, "net.jitter.us", jitter});
}
