#include "sinusoidal_traffic_pattern.h"
#include "metric_utils.h"
#include <cmath>
#include <chrono>
#include <iostream>

SinusoidalTrafficPattern::SinusoidalTrafficPattern(ClientContext &context, SendFunction sendFunc)
    : context_(context),
      sendFunc_(std::move(sendFunc)),
      timer_(context.ioContext),
      amplitude_(context.scenario.amplitude), // Mbps
      frequency_(context.scenario.frequency), // Hz
      base_rate_(context.rateMbps()), // center rate of the sinusoidal wave
      interval_ms_(static_cast<int>(context.intervalSec() * 1000)),
      elapsed_ms_(0) {
}

void SinusoidalTrafficPattern::start() {
    startTime_ = std::chrono::steady_clock::now();
    nextPlannedTime_ = startTime_;
    context_.metricCollector->start(startTime_);

    std::cout << "[INFO] Starting sinusoidal traffic generation..." << std::endl;
    sendData();
}

void SinusoidalTrafficPattern::sendData() {
    if (elapsed_ms_ >= context_.durationSec() * 1000) {
        context_.metricCollector->stop();
        std::cout << "[INFO] Sinusoidal traffic generation finished." << std::endl;
        return;
    }

    double time_sec = elapsed_ms_ / 1000.0;
    double current_rate_mbps = base_rate_ + amplitude_ * std::sin(2 * M_PI * frequency_ * time_sec);
    if (current_rate_mbps < 0) current_rate_mbps = 0.0;

    double bytes_per_sec = (current_rate_mbps * 1'000'000) / 8.0;
    int bytes_to_send = static_cast<int>((bytes_per_sec * interval_ms_) / 1000.0);

    std::string payload(bytes_to_send, 'x');
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
            self->sendData();
        }
    });
}
