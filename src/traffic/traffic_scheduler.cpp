// src/traffic_scheduler.cpp
#include "traffic_scheduler.h"
#include <algorithm>
#include <logger.h>

TrafficScheduler::TrafficScheduler(boost::asio::io_context &io,
                                   INetworkClient &client,
                                   const ClientContext &context,
                                   ITrafficPattern &pattern,
                                   std::function<void()> onFinish)
    : io_(io),
      client_(client),
      context_(context),
      pattern_(pattern),
      timer_(io_),
      packetBuffer_(context.traffic.packetSize - sizeof(int64_t), 0xAB),
      onFinish_(std::move(onFinish)) {
    maxPackets_ = pattern_.computeMaxPackets(
        context_.traffic.durationSec,
        context_.traffic.packetSize);

    constexpr size_t headerSize = sizeof(int64_t);
    const size_t payloadSize = packetBuffer_.size();
    sendBuffer_.resize(headerSize + payloadSize);

    std::memcpy(
        &sendBuffer_[headerSize],
        packetBuffer_.data(),
        payloadSize);
    nextTime_ = std::chrono::steady_clock::now();
}

void TrafficScheduler::start() {
    startTime_ = std::chrono::steady_clock::now();
    nextTime_ = startTime_;
    packetsSent_ = 0;

    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto ec) { sendOne(ec); });
}

void TrafficScheduler::sendOne(const boost::system::error_code &ec) {
    if (ec) return;

    if (packetsSent_ >= maxPackets_) {
        Logger::instance().info("Max packets reached, finishing traffic.");
        if (onFinish_) onFinish_();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    if (elapsed >= context_.traffic.durationSec) {
        Logger::instance().info("Traffic duration reached, finishing traffic.");
        if (onFinish_) onFinish_();
        return;
    }

    int64_t embeddedUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::memcpy(&sendBuffer_[0], &embeddedUs, sizeof(int64_t));
    client_.send(sendBuffer_);
    ++packetsSent_;

    const auto delay = pattern_.generateDelay(
        context_.traffic.packetSize,
        context_.traffic.intervalSec);

    nextTime_ += delay;
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto ec) { sendOne(ec); });
}

double TrafficScheduler::getTimestampInSeconds(const std::chrono::system_clock::time_point timePoint) {
    return std::chrono::duration_cast<std::chrono::duration<double> >(timePoint.time_since_epoch()).count();
}
