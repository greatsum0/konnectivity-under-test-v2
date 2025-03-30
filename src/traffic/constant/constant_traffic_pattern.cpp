#include "constant_traffic_pattern.h"
#include "metric_utils.h"
#include <chrono>
#include <iostream>

ConstantTrafficPattern::ConstantTrafficPattern(ClientContext& context, SendFunction sendFunc)
    : context_(context),
      sendFunc_(std::move(sendFunc)),
      timer_(context.ioContext),
      interval_ms_(static_cast<int>(context.intervalSec() * 1000)),
      elapsed_ms_(0) {

    const double bytes_per_sec = (context.rateMbps() * 1'000'000) / 8.0;
    bytes_per_interval_ = static_cast<int>((bytes_per_sec * interval_ms_) / 1000.0);
}

void ConstantTrafficPattern::start() {
    context_.metricCollector->start();
    std::cout << "[INFO] Constant traffic generation started." << std::endl;
    sendData();
}

void ConstantTrafficPattern::sendData() {
    if (elapsed_ms_ >= context_.durationSec() * 1000) {
        context_.metricCollector->stop();
        std::cout << "[INFO] Constant traffic generation finished." << std::endl;
        return;
    }

    std::string data(bytes_per_interval_, 'x');
    sendFunc_(data);
    context_.metricCollector->addBytesTransferred(bytes_per_interval_);
    elapsed_ms_ += interval_ms_;

    timer_.expires_after(std::chrono::milliseconds(interval_ms_));
    auto self = shared_from_this();
    timer_.async_wait([self](const asio::error_code& ec) {
        if (!ec) {
            self->sendData();
        }
    });
}
