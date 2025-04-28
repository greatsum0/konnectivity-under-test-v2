#pragma once

#include <vector>
#include <cstdint>


class ITrafficPattern {
public:
    virtual ~ITrafficPattern() = default;

    virtual std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) = 0;
    [[nodiscard]] virtual std::size_t computeMaxPackets(double durationSec, size_t packetSize) const = 0;
};