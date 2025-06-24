#include "packet_aggregator.h"
#include <logger.h>

PacketAggregator::PacketAggregator(double intervalSec)
    : intervalSec_(intervalSec) {
}

void PacketAggregator::addPacket(const PacketRecord &packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    packetLog_.emplace_back(packet);
}

AggregateSample PacketAggregator::sample() {
    AggregateSample agg{};
    std::vector<PacketRecord> snap; {
        std::lock_guard<std::mutex> lock(mutex_);
        snap.swap(packetLog_);
    }
    std::vector<int64_t> latencies;
    int64_t latencySumUs = 0;
    for (auto &pkt: snap) {
        if (pkt.isOut) {
            agg.bytesOut += pkt.size;
            agg.packetsOut++;
        } else {
            agg.bytesIn += pkt.size;
            agg.packetsIn++;
        }
        int64_t latUs = pkt.timestamp - pkt.embeddedTimestamp;
        latencies.push_back(latUs);
        latencySumUs += latUs;
    }
    const double totalBits = static_cast<double>(agg.bytesIn + agg.bytesOut) * 8.0;
    agg.throughputMbps = totalBits / (intervalSec_ * 1000000.0);
    if (!latencies.empty()) {
        const int64_t meanLatUs = latencySumUs / static_cast<int64_t>(latencies.size());
        agg.latency = meanLatUs;
        double jitterSum = 0.0;
        for (const auto &l: latencies) {
            jitterSum += static_cast<double>(std::abs(l - meanLatUs));
        }
        agg.jitterUs = jitterSum / static_cast<double>(latencies.size());
    } else {
        agg.latency = 0;
        agg.jitterUs = 0.0;
    }
    return agg;
}
