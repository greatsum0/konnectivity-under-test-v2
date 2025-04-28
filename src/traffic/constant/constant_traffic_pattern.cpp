#include "constant_traffic_pattern.h"
#include <cmath>

ConstantTrafficPattern::ConstantTrafficPattern(double rateMbps)
    : rateMbps_(rateMbps)
{}

std::vector<uint8_t> ConstantTrafficPattern::generatePayload(double intervalSec) {
    const double bits = rateMbps_ * 1'000'000.0 * intervalSec;
    const auto byteCount = static_cast<size_t>(std::round(bits / 8.0));
    return std::vector<uint8_t>(byteCount, 0xAB);
}

std::chrono::microseconds ConstantTrafficPattern::generateDelay(double intervalSec) {
    /// find the area under the graph that gives the total amount of data to transmit
    /// rateMbps is the speed of the data in bits per second
    /// duration is the time in seconds

    constexpr auto packetSize = 16*1024; // 16 KiB
    const double bits = rateMbps_ * 1'000'000.0 * intervalSec;
    const double bytes = std::round(bits / 8.0);
    const double packets = std::round(bytes / packetSize);

    const double delayMiliseconds = (intervalSec / packets) * 1'000.0;

    const double delayMicroseconds = delayMiliseconds * 1'000.0;




    return std::chrono::microseconds(static_cast<long long>(delayMicroseconds));
}