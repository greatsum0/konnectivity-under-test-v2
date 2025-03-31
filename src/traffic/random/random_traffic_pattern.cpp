#include "random_traffic_pattern.h"
#include "metric_utils.h"
#include <iostream>
#include <chrono>

RandomTrafficPattern::RandomTrafficPattern(ClientContext &context, SendFunction sendFunc)
    : context_(context),
      sendFunc_(std::move(sendFunc)),
      timer_(context.ioContext),
      interval_ms_(static_cast<int>(context.intervalSec() * 1000)),
      elapsed_ms_(0),
      max_rate_mbps_(context.scenario.max_rate_mbps),
      rng_(context.scenario.random_seed),
      rate_dist_(0, context.scenario.max_rate_mbps) {
}

void RandomTrafficPattern::start() {
    startTime_ = std::chrono::steady_clock::now();
    nextPlannedTime_ = startTime_;
    context_.metricCollector->start(startTime_);

    std::cout << "[INFO] Starting random traffic generation..." << std::endl;
    scheduleNextSend();
}

void RandomTrafficPattern::scheduleNextSend() {
    if (elapsed_ms_ >= context_.durationSec() * 1000) {
        context_.metricCollector->stop();
        std::cout << "[INFO] Random traffic generation finished." << std::endl;
        return;
    }

    int random_rate = rate_dist_(rng_);  // Mbps
    double bytes_per_sec = (random_rate * 1'000'000) / 8.0;
    int bytes_to_send = static_cast<int>((bytes_per_sec * interval_ms_) / 1000.0);

    std::string payload(bytes_to_send, 'r');
    sendFunc_(payload);
    context_.metricCollector->addBytesTransferred(bytes_to_send);

    elapsed_ms_ += interval_ms_;
    nextPlannedTime_ += std::chrono::milliseconds(interval_ms_);

    auto now = std::chrono::steady_clock::now();
    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
        nextPlannedTime_ > now ? (nextPlannedTime_ - now) : std::chrono::milliseconds(0)
    );

    timer_.expires_after(delay);
    auto self = shared_from_this();
    timer_.async_wait([self](const asio::error_code &ec) {
        if (!ec) {
            self->scheduleNextSend();
        }
    });
}
