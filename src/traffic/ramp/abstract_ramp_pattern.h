// abstract_ramp_pattern.h
#pragma once

#include "itraffic_pattern.h"


class AbstractRampPattern : public ITrafficPattern {
public:
    /// @param targetRateMbps  The peak rate (Mbps) at the end of the ramp
    /// @param rampTimeSec  How many seconds the ramp should take
    AbstractRampPattern(double targetRateMbps, double rampTimeSec);

    /// Estimate how many fixed-size packets you’ll send over durationSec.
    /// Uses the *average* rate over the ramp (i.e. half the peak if duration==rampTime)
    [[nodiscard]]
    std::size_t computeMaxPackets(double durationSec,
                                  std::size_t packetSize) const override;

    /// Compute inter-packet delay based on the *current* rate at this moment.
    std::chrono::microseconds generateDelay(size_t packetSize, double intervalSec) override;

protected:
    /// Returns the current instantaneous rate (Mbps).
    /// Subclasses implements its own ramp shape here.
    [[nodiscard]] virtual double currentRateFactor(double elapsedSec) const = 0;

    double targetRateMbps;
    double rampTimeSec_;
    std::chrono::steady_clock::time_point startTime_;
};
