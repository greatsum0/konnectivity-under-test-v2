#pragma once

#include "abstract_burst_pattern.h"

class PureBurstTrafficPattern : public AbstractBurstPattern {
public:
    PureBurstTrafficPattern(double burstSizeMb,
                            double burstDurationSec,
                            double burstIntervalSec);

    std::vector<uint8_t> generatePayload(double intervalSec) override;
};
