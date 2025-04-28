#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cstdint>


class ConstantTrafficPattern : public ITrafficPattern {
public:
    explicit ConstantTrafficPattern(double rateMbps);

    [[nodiscard]] std::size_t computeMaxPackets(double durationSec, size_t packetSize) const override;
    std::chrono::microseconds generateDelay(size_t packetSize) override;
private:
    double rateMbps_;
};
