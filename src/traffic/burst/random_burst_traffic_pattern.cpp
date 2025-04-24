// src/traffic/RandomBurstTrafficPattern.cpp
#include "random_burst_traffic_pattern.h"
#include <cmath>

RandomBurstTrafficPattern::RandomBurstTrafficPattern(unsigned seed,
                                                     double burstSizeMinMb,
                                                     double burstSizeMaxMb,
                                                     double burstDurationMinSec,
                                                     double burstDurationMaxSec,
                                                     double burstIntervalMinSec,
                                                     double burstIntervalMaxSec)
    : rng_(seed),
      sizeDist_(burstSizeMinMb * 1e6, burstSizeMaxMb * 1e6),
      durDist_(burstDurationMinSec, burstDurationMaxSec),
      intDist_(burstIntervalMinSec, burstIntervalMaxSec),
      elapsed_(0),
      nextBurstTime_(intDist_(rng_)),
      currentBurstDuration_(0)
{}

std::vector<uint8_t> RandomBurstTrafficPattern::generatePayload(double intervalSec) {
    elapsed_ += intervalSec;
    std::vector<uint8_t> buf;

    if (elapsed_ < nextBurstTime_) {
        return buf;
    }

    if (currentBurstDuration_ <= 0) {
        currentBurstDuration_ = durDist_(rng_);
    }

    if (currentBurstDuration_ > 0) {
        size_t bytes = static_cast<size_t>(std::round(sizeDist_(rng_)));
        buf.insert(buf.end(), bytes, 0xAA);
        currentBurstDuration_ -= intervalSec;
    }

    if (currentBurstDuration_ <= 0) {
        nextBurstTime_ = elapsed_ + intDist_(rng_);
    }

    return buf;
}
