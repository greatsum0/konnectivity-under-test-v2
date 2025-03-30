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
    context_.metricCollector->start();
    std::cout << "[INFO] Burst traffic generation started." << std::endl;
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
    timer_.expires_after(std::chrono::milliseconds(interval_ms_));
    auto self = shared_from_this();
    timer_.async_wait([self](const asio::error_code& ec) {
        if (!ec) {
            self->scheduleNextSend();
        }
    });
}
