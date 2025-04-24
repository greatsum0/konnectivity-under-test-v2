#include "sinusoidal_traffic_pattern.h"

SinusoidalTrafficPattern::SinusoidalTrafficPattern(double baseRateMbps,
                                                   double amplitudeMbps,
                                                   double frequencyHz)
  : baseRateMbps_(baseRateMbps),
    amplitudeMbps_(amplitudeMbps),
    omega_(2.0 * M_PI * frequencyHz),
    elapsedTime_(0.0)
{}

std::vector<uint8_t> SinusoidalTrafficPattern::generatePayload(double intervalSec) {
    // Advance the internal clock
    elapsedTime_ += intervalSec;

    // Compute instantaneous rate
    double instRate = baseRateMbps_ + amplitudeMbps_ * std::sin(omega_ * elapsedTime_);
    if (instRate < 0.0) instRate = 0.0;

    // Convert Mbps → bits in this interval
    double bits = instRate * 1e6 * intervalSec;

    // Convert bits → bytes
    size_t bytes = static_cast<size_t>(std::round(bits / 8.0));

    // Fill buffer with a constant pattern byte
    return std::vector<uint8_t>(bytes, 0xBE);
}
