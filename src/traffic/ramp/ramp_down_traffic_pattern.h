#pragma once

#include "abstract_ramp_pattern.h"

/// Linearly ramps *down* from targetRateMbps → 0 over rampTimeSec
class RampDownTrafficPattern final : public AbstractRampPattern {
public:
    /// @param targetRateMbps  Starting rate in Mb/s at time zero
    /// @param rampTimeSec  Duration of ramp in seconds
    RampDownTrafficPattern(double targetRateMbps, double rampTimeSec);

protected:
    /// Returns (1 – elapsed/rampTimeSec), clamped to [0,1]
    [[nodiscard]] double currentRateFactor(double elapsedSec) const override;
};
