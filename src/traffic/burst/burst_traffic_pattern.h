#pragma once
#include "abstract_burst_pattern.h"

class BurstTrafficPattern : public AbstractBurstPattern {
public:
    BurstTrafficPattern(double rateMbps,
                        double burstSizeMb,
                        double burstDurationSec,
                        double burstIntervalSec);

    std::vector<uint8_t> generatePayload(double intervalSec) override;

private:
    double rateMbps_;
};
