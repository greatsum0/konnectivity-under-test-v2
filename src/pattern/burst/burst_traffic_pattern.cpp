#include "burst_traffic_pattern.h"
#include <cmath>

BurstTrafficPattern::BurstTrafficPattern(const double meanRateMbps,
                                         const double burstSizeMB,
                                         const double burstDurationSec,
                                         const double burstIntervalSec)
    : meanRateMbps_(meanRateMbps),
      burstSizeMB_(burstSizeMB),
      burstDurationSec_(burstDurationSec),
      burstIntervalSec_(burstIntervalSec) {
}

std::size_t BurstTrafficPattern::computeMaxPackets(const double durationSec, const size_t packetSize) const {
    // compute extra burst rate (Mbps) over burstDurationSec_
    const double burstRateMbps = (burstSizeMB_ * 8.0) / burstDurationSec_;
    // fraction of time in burst
    const double fracBurst = std::min(1.0, burstDurationSec_ / burstIntervalSec_);
    // average Mbps = mean + burst fraction × burst rate
    const double avgRate = meanRateMbps_ + fracBurst * burstRateMbps;
    const double totalBits = avgRate * 1e6 * durationSec;
    const double bitsPerPkt = static_cast<double>(packetSize) * 8.0;
    return static_cast<std::size_t>(std::ceil(totalBits / bitsPerPkt));
}

std::chrono::microseconds BurstTrafficPattern::generateDelay(const size_t packetSize, const double intervalSec) {
    using namespace std::chrono;
    // elapsed time
    const double elapsed = duration<double>(
        steady_clock::now() - startTime_).count();
    // phase in burst cycle
    const double phase = std::fmod(elapsed, burstIntervalSec_);
    const bool inBurst = (phase < burstDurationSec_);
    // current rate
    const double burstRateMbps = (burstSizeMB_ * 8.0) / burstDurationSec_;
    double currRate = meanRateMbps_ + (inBurst ? burstRateMbps : 0.0);
    currRate = std::max(currRate, 1e-9);
    // bits this interval
    const double bits = currRate * 1e6 * intervalSec;
    const double bytes = bits / 8.0;
    double packets = std::round(bytes / static_cast<double>(packetSize));
    if (packets < 1.0) packets = 1.0;
    const double dtSec = intervalSec / packets;
    return duration_cast<microseconds>(duration<double>(dtSec));
}
