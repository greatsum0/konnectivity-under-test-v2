#include "packet_aggregator.h"

PacketAggregator::PacketAggregator(double intervalSec)
    : intervalSec_(intervalSec) {
}

void PacketAggregator::addPacket(const PacketRecord &packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    packetLog_.emplace_back(packet);
}

AggregateSample PacketAggregator::sample() {
    AggregateSample agg{};
    agg.timestamp = std::chrono::system_clock::now();

    std::vector<PacketRecord> snapshot; {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot.swap(packetLog_);
    }

    std::chrono::system_clock::duration latencySum = {};
    for (const auto &pkt: snapshot) {
        if (pkt.isOut) {
            agg.bytesOut += pkt.size;
            agg.packetsOut++;
        } else {
            agg.bytesIn += pkt.size;
            agg.packetsIn++;
        }
        latencySum += pkt.timestamp - pkt.embeddedTimestamp;
    }

    const double totalBits = static_cast<double>(agg.bytesIn + agg.bytesOut) * 8.0;
    agg.throughputMbps = totalBits / (intervalSec_ * 1'000'000.0);

    // Calculate average latency if there are packets
    if (!snapshot.empty()) {
        agg.latency = latencySum / snapshot.size();
    } else {
        agg.latency = std::chrono::system_clock::duration::zero();
    }

    return agg;
}
