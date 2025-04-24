#include "random_traffic_pattern.h"

RandomTrafficPattern::RandomTrafficPattern(unsigned seed, double maxRateMbps)
  : rng_(seed),
    distRate_(0.0, maxRateMbps),
    maxRateMbps_(maxRateMbps)
{}

std::vector<uint8_t> RandomTrafficPattern::generatePayload(double intervalSec) {
    // Pick a random rate in Mbps
    double rate = distRate_(rng_);
    // Convert to bits over this interval
    double bits = rate * 1e6 * intervalSec;
    // Convert bits → bytes
    size_t bytes = static_cast<size_t>(std::round(bits / 8.0));
    // Fill return buffer with a constant byte value
    return std::vector<uint8_t>(bytes, 0xFA);
}
