// traffic_scheduler.cpp
#include "traffic_scheduler.h"

TrafficScheduler::TrafficScheduler(asio::io_context &io,
                                   INetworkClient &client,
                                   const ClientContext &context,
                                   ITrafficPattern &pattern)
    : io_(io),
      client_(client),
      context_(context), pattern_(pattern), timer_(io_),
      packetBuffer_(context.traffic.packetSize, 0xAB) {
    // 3) set our first "wake" to now
    maxPackets_ = pattern_.computeMaxPackets(context_.traffic.durationSec,
                                             context_.traffic.packetSize);
    nextTime_ = std::chrono::steady_clock::now();
}

void TrafficScheduler::start() {
    // schedule the very first send
    timer_.expires_at(nextTime_);
    timer_.async_wait(
        [this](auto ec) { sendOne(ec); }
    );
}

void TrafficScheduler::sendOne(const asio::error_code &ec) {
    if (ec) return;

    // check elapsed time
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();
    if (elapsed >= context_.traffic.durationSec) {
        return; // done
    }

    // 1) ask pattern for this interval's payload and send it
    client_.send(packetBuffer_);

    // 2) ask pattern how long until the *next* send
    const auto delay = pattern_.generateDelay(context_.traffic.packetSize,
                                               context_.traffic.intervalSec);

    // 3) advance our absolute nextTime_ and schedule
    nextTime_ += delay;
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto ec) { sendOne(ec); });
}
