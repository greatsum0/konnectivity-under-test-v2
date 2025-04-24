#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cstdint>

class AbstractRampPattern : public ITrafficPattern {
public:
    AbstractRampPattern(double maxRateMbps, double rampTimeSec);

    // Generate payload for the given interval.
    std::vector<uint8_t> generatePayload(double intervalSec) override;

protected:
    // Subclasses implement to compute current rate at elapsed time.
    virtual double calculateRate(double elapsedSec) const = 0;

    double maxRateMbps_; //< top rate in Mbps
    double rampTimeSec_; //< ramp duration in seconds
    double elapsedSec_; //< accumulated time since start
};
