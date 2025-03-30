#include "abstract_ramp_pattern.h"
#include "metric_utils.h"
#include <iostream>
#include <chrono>

AbstractRampPattern::AbstractRampPattern(ClientContext& context, SendFunction sendFunc)
    : context_(context),
      sendFunc_(std::move(sendFunc)),
      timer_(context.ioContext),
      interval_ms_(static_cast<int>(context.intervalSec() * 1000)),
      elapsed_ms_(0)
{
}

void AbstractRampPattern::start() {
    context_.metricCollector->start();
    std::cout << "[INFO] Starting ramp traffic generation..." << std::endl;
    scheduleNextSend();
}

void AbstractRampPattern::scheduleNextSend() {
    if (elapsed_ms_ >= context_.durationSec() * 1000) {
        context_.metricCollector->stop();
        std::cout << "[INFO] Ramp traffic generation finished." << std::endl;
        return;
    }

    int current_rate_mbps = calculateRateMbps(elapsed_ms_);
    double bytes_per_sec = (current_rate_mbps * 1'000'000) / 8.0;
    int bytes_to_send = static_cast<int>((bytes_per_sec * interval_ms_) / 1000.0);

    std::string payload(bytes_to_send, 'r');
    sendFunc_(payload);
    context_.metricCollector->addBytesTransferred(bytes_to_send);

    elapsed_ms_ += interval_ms_;
    timer_.expires_after(std::chrono::milliseconds(interval_ms_));
    auto self = shared_from_this();
    timer_.async_wait([self](const asio::error_code& ec) {
        if (!ec) {
            self->scheduleNextSend();
        }
    });
}
