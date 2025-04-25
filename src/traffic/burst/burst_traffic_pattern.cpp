#include "burst_traffic_pattern.h"

BurstTrafficPattern::BurstTrafficPattern(double rateMbps,
                                         double burstSizeMb,
                                         double burstDurationSec,
                                         double burstIntervalSec)
  : AbstractBurstPattern(burstSizeMb, burstDurationSec, burstIntervalSec),
    rateMbps_(rateMbps)
{}

std::vector<uint8_t> BurstTrafficPattern::generatePayload(double intervalSec) {
    const double bits = rateMbps_ * 1e6 * intervalSec;
    const auto baseBytes = static_cast<size_t>(std::round(bits / 8.0));
    std::vector<uint8_t> basePayload(baseBytes, 0xAB);

    if (isInBurst(intervalSec)) {
        auto burstPayload = generateBurstPayload();
        basePayload.insert(basePayload.end(), burstPayload.begin(), burstPayload.end());
    }
    return basePayload;
}
