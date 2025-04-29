#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cstdint>
#include <chrono>

class ConstantTrafficPattern final : public ITrafficPattern {
public:
    /// @param rateMbps constant megabits/sec
    explicit ConstantTrafficPattern(double rateMbps);

    /// Total packets = ceil( totalBits / bitsPerPacket )
    [[nodiscard]] std::size_t computeMaxPackets(double durationSec, size_t packetSize) const override;

    /// Delay until next send, based on constant rate
    std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) override;
private:
    double rateMbps_;
};
