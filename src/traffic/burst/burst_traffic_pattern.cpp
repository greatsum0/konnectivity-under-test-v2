#include "burst_traffic_pattern.h"

BurstTrafficPattern::BurstTrafficPattern(double rateMbps,
                                         double burstSizeMb,
                                         double burstDurationSec,
                                         double burstIntervalSec)
  : AbstractBurstPattern(burstSizeMb, burstDurationSec, burstIntervalSec),
    rateMbps_(rateMbps)
{}

std::vector<uint8_t> BurstTrafficPattern::generatePayload(double intervalSec) {
    double bits = rateMbps_ * 1e6 * intervalSec;
    size_t baseBytes = static_cast<size_t>(std::round(bits / 8.0));
    std::vector<uint8_t> buf(baseBytes, 0xAB);

    if (isInBurst(intervalSec)) {
        auto b = generateBurstPayload();
        buf.insert(buf.end(), b.begin(), b.end());
    }
    return buf;
}
