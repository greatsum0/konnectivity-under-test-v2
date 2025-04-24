#include "pure_burst_traffic_pattern.h"

PureBurstTrafficPattern::PureBurstTrafficPattern(double burstSizeMb,
                                                 double burstDurationSec,
                                                 double burstIntervalSec)
  : AbstractBurstPattern(burstSizeMb, burstDurationSec, burstIntervalSec)
{}

std::vector<uint8_t> PureBurstTrafficPattern::generatePayload(double intervalSec) {
    if (isInBurst(intervalSec)) {
        return generateBurstPayload();
    }
    return {};
}
