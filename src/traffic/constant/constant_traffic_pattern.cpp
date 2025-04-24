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
