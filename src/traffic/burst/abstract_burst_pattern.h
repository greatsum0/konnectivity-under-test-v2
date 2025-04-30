#pragma once

#include "itraffic_pattern.h"
#include <chrono>

/// Base class for all burst patterns: fixed payload size + start time.
class AbstractBurstPattern : public ITrafficPattern {
public:
    explicit AbstractBurstPattern();

protected:
    std::chrono::steady_clock::time_point startTime_;
};
