#include "random_burst_traffic_pattern.h"
#include <cmath>
#include <iostream>

RandomBurstTrafficPattern::RandomBurstTrafficPattern(const unsigned seed,
                                                     const double rateMbps,
                                                     const double burstSizeMinMb,
                                                     const double burstSizeMaxMb,
                                                     const double burstDurationMinMs,
                                                     const double burstDurationMaxMs,
                                                     const double burstIntervalMinMs,
                                                     const double burstIntervalMaxMs)
    : rateMbps_(rateMbps),
      rng_(seed),
      sizeDist_(burstSizeMinMb * 1e6, burstSizeMaxMb * 1e6),
      durDist_(burstDurationMinMs / 1000.0, burstDurationMaxMs / 1000.0),
      intDist_(burstIntervalMinMs / 1000.0, burstIntervalMaxMs / 1000.0),
      elapsed_(0),
      nextBurstTime_(intDist_(rng_)),
      currentBurstDuration_(0),
      remainingBurstBytes_(0) {
}

std::vector<uint8_t> RandomBurstTrafficPattern::generatePayload(const double intervalSec) {
    elapsed_ += intervalSec;
    std::vector<uint8_t> payload;

    //constant traffic
    const double baseBits = rateMbps_ * 1e6 * intervalSec;
    const auto baseBytes = static_cast<size_t>(baseBits / 8.0);
    payload.insert(payload.end(), baseBytes, 0xAA);

    // if burst is active, add burst payload
    if (elapsed_ >= nextBurstTime_ && currentBurstDuration_ <= 0) {
        currentBurstDuration_ = durDist_(rng_);
        remainingBurstBytes_ = static_cast<size_t>(std::round(sizeDist_(rng_)));

        if (currentBurstDuration_ < 0.001)
            currentBurstDuration_ = 0.001; // dont fall below 1ms

        nextBurstTime_ = elapsed_ + intDist_(rng_);
    }

    // if burst is active, add burst payload
    if (currentBurstDuration_ > 0 && remainingBurstBytes_ > 0) {
        auto burstBytes = static_cast<size_t>(
            remainingBurstBytes_ * (intervalSec / currentBurstDuration_));

        burstBytes = std::min(burstBytes, remainingBurstBytes_);
        payload.insert(payload.end(), burstBytes, 0xDD);

        remainingBurstBytes_ -= burstBytes;
        currentBurstDuration_ -= intervalSec;

        if (remainingBurstBytes_ == 0 || currentBurstDuration_ <= 0) {
            currentBurstDuration_ = 0;
        }
    }

    return payload;
}
