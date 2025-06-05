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

    std::vector<PacketRecord> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot.swap(packetLog_);
    }

    std::vector<std::chrono::microseconds> latencies;
    std::chrono::system_clock::duration latencySum = {};
    for (const auto &pkt: snapshot) {
        if (pkt.isOut) {
            agg.bytesOut += pkt.size;
            agg.packetsOut++;
        } else {
            agg.bytesIn += pkt.size;
            agg.packetsIn++;
        }

        const auto latency = pkt.timestamp - pkt.embeddedTimestamp;
        latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(latency));
        latencySum += latency;
    }

    const double totalBits = static_cast<double>(agg.bytesIn + agg.bytesOut) * 8.0;
    agg.throughputMbps = totalBits / (intervalSec_ * 1'000'000.0);

    // Jitter is calculated as the Mean Absolute Deviation (MAD) of one-way latencies.
    // Let L = {l₁, l₂, ..., lₙ} be the set of one-way latencies in microseconds.
    // Let μ = (1/n) * Σ lᵢ  be the mean latency.
    // Then, jitter = (1/n) * Σ |lᵢ - μ|
    // This provides a robust, intuitive measure of latency variation per RFC 5481 principles.

    if (!latencies.empty()) {
        agg.latency = latencySum / snapshot.size();

        // Mean absolute deviation (Jitter)
        const auto meanLatency = std::chrono::duration_cast<std::chrono::microseconds>(agg.latency).count();
        double jitterSum = 0.0;

        for (const auto &lat : latencies) {
            jitterSum += std::abs(lat.count() - meanLatency);
        }

        agg.jitterUs = jitterSum / latencies.size();
    } else {
        agg.latency = std::chrono::system_clock::duration::zero();
        agg.jitterUs = 0.0;
    }

    return agg;
}
