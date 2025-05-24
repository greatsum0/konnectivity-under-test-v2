#include "sinusoidal_traffic_pattern.h"
#include <cmath>

SinusoidalTrafficPattern::SinusoidalTrafficPattern(const double meanRateMbps,
                                                   const double amplitudeMbps,
                                                   const double frequencyHz)
    : meanRateMbps_(meanRateMbps),
      amplitudeMbps_(amplitudeMbps),
      frequencyHz_(frequencyHz),
      startTime_(std::chrono::steady_clock::now()) {
}

std::size_t
SinusoidalTrafficPattern::computeMaxPackets(const double durationSec,
                                            const std::size_t packetSize) const {
    // Over a full sine cycle the average of sin() is zero,
    // so avg rate = meanRateMbps_.
    const double totalBits = meanRateMbps_ * 1e6 * durationSec;
    const double bitsPerPkt = static_cast<double>(packetSize) * 8.0;
    return static_cast<std::size_t>(
        std::ceil(totalBits / bitsPerPkt)
    );
}

std::chrono::microseconds
SinusoidalTrafficPattern::generateDelay(size_t packetSize, const double intervalSec) {
    using namespace std::chrono;

    // 1) figure out elapsed time
    const double elapsed = duration<double>(
        steady_clock::now() - startTime_
    ).count();

    // 2) compute instantaneous rate
    const double rate = currentRateMbps(elapsed);

    // 3) bits to send in this interval
    const double bits = rate * 1e6 * intervalSec;

    // 4) how many fixed‐size packets?
    const double bytes = bits / 8.0;
    double packets = std::round(bytes / static_cast<double>(packetSize));
    if (packets < 1.0) packets = 1.0;

    // 5) evenly spaced over the interval
    const double dtSec = intervalSec / packets;
    return duration_cast<microseconds>(duration<double>(dtSec));
}

double
SinusoidalTrafficPattern::currentRateMbps(const double elapsedSec) const {
    // r(t) = mean + amplitude * sin(2π f t)
    const double omega = 2.0 * M_PI * frequencyHz_;
    const double val = meanRateMbps_ +
                       amplitudeMbps_ * std::sin(omega * elapsedSec);
    return std::max(0.0, val);
}
