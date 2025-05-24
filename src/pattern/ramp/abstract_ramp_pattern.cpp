#include "abstract_ramp_pattern.h"
#include <cmath>
#include <chrono>
AbstractRampPattern::AbstractRampPattern(const double targetRateMbps,
                                         const double rampTimeSec)
    : targetRateMbps(targetRateMbps),
      rampTimeSec_(rampTimeSec),
      startTime_(std::chrono::steady_clock::now()) {
}

std::size_t
AbstractRampPattern::computeMaxPackets(const double durationSec,
                                       const std::size_t packetSize) const {
    // Compute the *total area* under the normalized ramp curve up to durationSec:
    // If durationSec >= rampTime, area = 0.5 * 1 * rampTime
    // If durationSec <  rampTime, area = 0.5 * (duration/rampTime)^2 * rampTime

    const double time = std::min(durationSec, rampTimeSec_);
    const double area = 0.5 * time * targetRateMbps;
    const double avgRateMbps = area / time; // ∫(0→t) (x/rampTime) dx

    // Total bits = avgRateMbps * 1e6 * durationSec
    const double totalBits = avgRateMbps * 1e6 * durationSec;
    const double bitsPerPacket = static_cast<double>(packetSize) * 8.0;
    return static_cast<std::size_t>(std::ceil(totalBits / bitsPerPacket));
}

std::chrono::microseconds
AbstractRampPattern::generateDelay(const size_t packetSize, const double intervalSec) {
    // elapsed time since start
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count();
    // get the normalized factor (0→1) from elapsed
    const double factor = currentRateFactor(elapsed); // to find the value in the y-axis
    const double currRateMbps = targetRateMbps * factor; // point at y-axis

    // Bits this interval
    const double bits = currRateMbps * 1e6 * intervalSec;
    // Packets needed (at least one)
    const double bytes = bits / 8.0;
    const double numPackets = std::max(1.0, std::round(bytes / packetSize));

    // spread sends evenly
    const double dtSec = intervalSec / numPackets;
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(dtSec));
}
