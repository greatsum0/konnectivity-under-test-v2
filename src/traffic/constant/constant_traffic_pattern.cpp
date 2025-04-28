#include "constant_traffic_pattern.h"
#include <cmath>

ConstantTrafficPattern::ConstantTrafficPattern(double rateMbps)
    : rateMbps_(rateMbps) {
}

std::size_t ConstantTrafficPattern::computeMaxPackets(double durationSec, const size_t packetSize) const {
    const double totalBits = rateMbps_ * durationSec * 1e6; // Convert Mbps to bits
    const double bitsPerPacket = static_cast<double>(packetSize) * 8.0;
    // ceil to cover the full bit‐budget
    return static_cast<std::size_t>(
        std::ceil(totalBits / bitsPerPacket)
    );
}

std::chrono::microseconds ConstantTrafficPattern::generateDelay(const size_t packetSize, double intervalSec) {
    const double bitsPerPacket = static_cast<double>(packetSize) * 8.0;
    const double rateBps = rateMbps_ * 1e6;
    const double dtSeconds = bitsPerPacket / rateBps;

    const auto packetDelay = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::duration<double>(dtSeconds));

    return packetDelay;
}
