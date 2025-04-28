#pragma once

#include <vector>
#include <cstdint>


class ITrafficPattern {
public:
    virtual ~ITrafficPattern() = default;

    virtual std::vector<uint8_t> generatePayload(double intervalSec) = 0;
    virtual std::chrono::microseconds generateDelay(double intervalSec) = 0;
};