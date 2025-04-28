#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cstdint>


class ConstantTrafficPattern : public ITrafficPattern {
public:
    explicit ConstantTrafficPattern(double rateMbps);

    std::vector<uint8_t> generatePayload(double intervalSec) override;
    std::chrono::microseconds generateDelay(double intervalSec) override;
private:
    double rateMbps_;
};
