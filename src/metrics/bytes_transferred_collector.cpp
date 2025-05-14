#include "bytes_transferred_collector.h"

BytesTransferredCollector::BytesTransferredCollector(std::vector<Metric> &store)
    : _store(store) {
}

void BytesTransferredCollector::onNetworkSample(
    std::chrono::system_clock::time_point ts,
    uint64_t bytesIn,
    uint64_t bytesOut,
    uint64_t /*packetsIn*/,
    uint64_t /*packetsOut*/
) {
    _store.push_back({ts, "bytes_in", static_cast<double>(bytesIn)});
    _store.push_back({ts, "bytes_out", static_cast<double>(bytesOut)});
}

void BytesTransferredCollector::onCpuSample(
    std::chrono::system_clock::time_point,
    double,
    double) {
    // no‐op
}
