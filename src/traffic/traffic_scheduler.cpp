// traffic_scheduler.cpp
#include "traffic_scheduler.h"

TrafficScheduler::TrafficScheduler(asio::io_context& io,
                                   INetworkClient& client,
                                   double rateMbps,
                                   size_t packetSize,
                                   double durationSec)
  : io_(io),
    timer_(io_),
    client_(client),
    packetBuffer_(packetSize, 0xAB)
{
    // 1) compute bits per packet, and inter‐packet dt
    const double bitsPerPacket = static_cast<double>(packetSize) * 8.0;
    const double rateBps        = rateMbps * 1e6;
    const double dtSeconds      = bitsPerPacket / rateBps;

    packetDelay_ = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::duration<double>(dtSeconds));

    // 2) compute how many packets to send in total
    const double totalBits = rateBps * durationSec;
    maxPackets_ = static_cast<size_t>(std::ceil(totalBits / bitsPerPacket));

    // 3) set our first "wake" to now
    nextTime_ = std::chrono::steady_clock::now();
}

void TrafficScheduler::start() {
    // schedule the very first send
    timer_.expires_at(nextTime_);
    timer_.async_wait(
        [this](auto ec){ sendOne(ec); }
    );
}

void TrafficScheduler::sendOne(const asio::error_code& ec) {
    if (ec || packetsSent_ >= maxPackets_) {
        // either cancelled or done
        return;
    }

    // 1) send the packet
    client_.send(packetBuffer_);
    ++packetsSent_;

    // 2) compute the next absolute send‐time
    nextTime_ += packetDelay_;

    // 3) schedule the next send
    timer_.expires_at(nextTime_);
    timer_.async_wait(
        [this](auto ec){ sendOne(ec); }
    );
}
