#pragma once

#include "abstract_ramp_pattern.h"

/// Linearly ramps *up* from 0 → targetRateMbps over rampTimeSec
class RampUpTrafficPattern final : public AbstractRampPattern {
public:
    /// @param targetRateMbps  Peak rate in Mb/s at end of ramp
    /// @param rampTimeSec  Duration of ramp in seconds
    RampUpTrafficPattern(double targetRateMbps, double rampTimeSec);

protected:
    /// Returns (elapsed / rampTimeSec), clamped to [0,1]
    [[nodiscard]] double currentRateFactor(double elapsedSec) const override;
};
