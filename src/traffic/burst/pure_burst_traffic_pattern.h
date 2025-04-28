#pragma once

#include "burst_traffic_pattern.h"
#include <cstddef>

/// Zero mean rate, only periodic bursts.
class PureBurstTrafficPattern final : public BurstTrafficPattern {
public:
    PureBurstTrafficPattern(const double burstSizeMB,
                            const double burstDurationSec,
                            const double burstIntervalSec)
        : BurstTrafficPattern(
            /*meanRateMbps=*/0.0,
                             burstSizeMB,
                             burstDurationSec,
                             burstIntervalSec
        ) {
    }
};
