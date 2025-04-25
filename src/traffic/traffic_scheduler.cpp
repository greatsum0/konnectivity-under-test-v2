#include "traffic_scheduler.h"
#include <chrono>
#include <thread>

TrafficScheduler::TrafficScheduler(asio::io_context &io,
                                   INetworkClient &client,
                                   ITrafficPattern &pattern,
                                   double intervalSec,
                                   double durationSec,
                                   size_t packetSize)
    : io_(io),
      timer_(io_),
      client_(client),
      pattern_(pattern),
      nextTime_(std::chrono::steady_clock::now()),
      interval_(intervalSec),
      currentIteration_(0),
      packetSize_(packetSize){
    maxIterations_ = static_cast<std::size_t>(durationSec / intervalSec);
}

void TrafficScheduler::start() {
    scheduleNext();
}

void TrafficScheduler::scheduleNext() {
    if (currentIteration_ >= maxIterations_)
        return;

    nextTime_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval_);
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](const asio::error_code &ec) {
        if (!ec) {
            auto payload = pattern_.generatePayload(interval_.count());

            for (size_t i = 0; i < payload.size(); i += packetSize_) {
                const size_t len = std::min(packetSize_, payload.size() - i);
                std::vector packet(payload.begin() + i, payload.begin() + i + len);
                client_.send(packet);
            }

            ++currentIteration_;
            scheduleNext();
        }
    });
}

