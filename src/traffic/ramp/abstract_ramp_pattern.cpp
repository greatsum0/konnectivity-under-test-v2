#include "abstract_ramp_pattern.h"
#include <cmath>

AbstractRampPattern::AbstractRampPattern(double maxRateMbps, double rampTimeSec)
  : maxRateMbps_(maxRateMbps),
    rampTimeSec_(rampTimeSec),
    elapsedSec_(0.0)
{}

std::vector<uint8_t> AbstractRampPattern::generatePayload(double intervalSec) {
    // Advance elapsed time
    elapsedSec_ += intervalSec;

    // Compute current rate (Mbps)
    double rate = calculateRate(elapsedSec_);
    if (rate < 0.0) rate = 0.0;

    // Convert rate to bytes for this interval
    double bits = rate * 1e6 * intervalSec;
    size_t bytes = static_cast<size_t>(std::round(bits / 8.0));

    // Fill buffer with a pattern byte
    return std::vector<uint8_t>(bytes, 0xAD);
}
