// src/traffic/AbstractBurstPattern.h
#pragma once

#include "itraffic_pattern.h"
#include <vector>
#include <cmath>

class AbstractBurstPattern : public ITrafficPattern {
protected:
    double burstSizeBytes_;
    double burstDurationSec_;
    double burstIntervalSec_;
    double elapsed_;

    AbstractBurstPattern(double burstSizeMb,
                         double burstDurationSec,
                         double burstIntervalSec)
      : burstSizeBytes_(burstSizeMb * 1e6),
        burstDurationSec_(burstDurationSec),
        burstIntervalSec_(burstIntervalSec),
        elapsed_(0)
    {}

    // if we are in the burst part of the interval return true
    bool isInBurst(double intervalSec) {
        elapsed_ += intervalSec;
        double mod = std::fmod(elapsed_, burstIntervalSec_);
        return mod < burstDurationSec_;
    }

    // Generates only the burst part. (0 or burstSizeBytes_)
    std::vector<uint8_t> generateBurstPayload() {
        return std::vector<uint8_t>(
            static_cast<size_t>(burstSizeBytes_), 0xCD);
    }
};
