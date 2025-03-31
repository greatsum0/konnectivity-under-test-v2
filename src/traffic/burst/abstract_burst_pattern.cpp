#include "abstract_burst_pattern.h"
#include "client_context.h"
#include "metric_utils.h"
#include <chrono>
#include <iostream>

AbstractBurstPattern::AbstractBurstPattern(ClientContext& context, SendFunction sendFunc)
    : context_(context),
      sendFunc_(std::move(sendFunc)),
      timer_(context.ioContext),
      interval_ms_(static_cast<int>(context.intervalSec() * 1000)),
      elapsed_ms_(0) {}

void AbstractBurstPattern::start() {
    std::cout << "[INFO] Burst traffic generation started." << std::endl;
    startTime_ = std::chrono::steady_clock::now();
    nextPlannedTime_ = startTime_;
    context_.metricCollector->start(startTime_);
    scheduleNextSend();
}


void AbstractBurstPattern::scheduleNextSend() {
    if (elapsed_ms_ >= context_.durationSec() * 1000) {
        context_.metricCollector->stop();
        std::cout << "[INFO] Burst traffic generation finished." << std::endl;
        return;
    }

    std::string payload = generatePayload();
    if (!payload.empty()) {
        sendFunc_(payload);
        context_.metricCollector->addBytesTransferred(static_cast<int>(payload.size()));
    }

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

