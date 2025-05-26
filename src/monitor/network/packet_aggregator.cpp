#include "packet_aggregator.h"

PacketAggregator::PacketAggregator(double intervalSec)
    : intervalSec_(intervalSec) {}

void PacketAggregator::addPacket(const PacketRecord &packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    packetLog_.emplace_back(packet);
}

AggregateSample PacketAggregator::sample() {
    AggregateSample agg{};
    agg.timestamp = std::chrono::system_clock::now();

    std::vector<PacketRecord> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot.swap(packetLog_);
    }

    for (const auto &pkt : snapshot) {
        if (pkt.isOut) {
            agg.bytesOut += pkt.size;
            agg.packetsOut++;
        } else {
            agg.bytesIn += pkt.size;
            agg.packetsIn++;
        }
    }

    const double totalBits = static_cast<double>(agg.bytesIn + agg.bytesOut) * 8.0;
    agg.throughputMbps = totalBits / (intervalSec_ * 1'000'000.0);

    return agg;
}
