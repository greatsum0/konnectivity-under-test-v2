// random_burst_traffic_pattern.cpp
#include "random_burst_traffic_pattern.h"
#include <cmath>

RandomBurstTrafficPattern::RandomBurstTrafficPattern(
    const double meanRateMbps,
    const double minBurstSizeMB,
    const double maxBurstSizeMB,
    const double minBurstDurationMs,
    const double maxBurstDurationMs,
    const double minBurstIntervalMs,
    const double maxBurstIntervalMs,
    const unsigned int seed)
    : AbstractBurstPattern(),
      meanRateMbps_(meanRateMbps),
      minBurstSizeMB_(minBurstSizeMB),
      maxBurstSizeMB_(maxBurstSizeMB),
      minBurstDurationMs_(minBurstDurationMs),
      maxBurstDurationMs_(maxBurstDurationMs),
      minBurstIntervalMs_(minBurstIntervalMs),
      maxBurstIntervalMs_(maxBurstIntervalMs),
      rng_(seed),
      sizeDist_(minBurstSizeMB_, maxBurstSizeMB_),
      durDist_(minBurstDurationMs_, maxBurstDurationMs_),
      intDist_(minBurstIntervalMs_, maxBurstIntervalMs_) {
}

std::size_t RandomBurstTrafficPattern::computeMaxPackets(const double durationSec, const std::size_t packetSize) const {
    // approximate using mean rate
    const double totalBits = meanRateMbps_ * 1e6 * durationSec;
    const double bitsPerPkt = static_cast<double>(packetSize) * 8.0;
    return static_cast<std::size_t>(
        std::ceil(totalBits / bitsPerPkt)
    );
}

std::chrono::microseconds RandomBurstTrafficPattern::generateDelay(const size_t packetSize, const double intervalSec) {
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();

    // start a new burst if needed
    if (now >= nextBurstStartSec_) {
        const double burstSizeMB = sizeDist_(rng_);
        double burstDurSec = durDist_(rng_) / 1e3;
        double nextIntSec = intDist_(rng_) / 1e3;

        currentBurstRateMbps_ = (burstSizeMB * 8.0) / burstDurSec;
        currentBurstEndSec_ = now + burstDurSec;
        nextBurstStartSec_ = now + nextIntSec;
    }

    bool inBurst = (now < currentBurstEndSec_);
    double rate = meanRateMbps_ + (inBurst ? currentBurstRateMbps_ : 0.0);
    rate = std::max(rate, 1e-9);

    double bits = rate * 1e6 * intervalSec;
    double bytes = bits / 8.0;
    double packets = std::round(bytes / static_cast<double>(packetSize));
    if (packets < 1.0) packets = 1.0;

    double dtSec = intervalSec / packets;
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(dtSec));
}
