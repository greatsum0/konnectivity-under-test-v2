// src/traffic_scheduler.cpp
#include "traffic_scheduler.h"

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
      packetBuffer_(context.traffic.packetSize, 0xAB),
      onFinish_(std::move(onFinish)) {
    maxPackets_ = pattern_.computeMaxPackets(
        context_.traffic.durationSec,
        context_.traffic.packetSize);

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

    // Stop if we sent too many packets
    if (packetsSent_ >= maxPackets_) {
        Logger::instance().info("Max packets reached, finishing traffic.");
        if (onFinish_) onFinish_();
        return;
    }

    // Stop if duration elapsed
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    if (elapsed >= context_.traffic.durationSec) {
        Logger::instance().info("Traffic duration reached, finishing traffic.");
        if (onFinish_) onFinish_();
        return;
    }

    // Send one packet
    //convert packet to string
    const std::string packetString(packetBuffer_.begin(), packetBuffer_.end());
    const auto sendTime = std::chrono::system_clock::now();
    const auto timestamp = getTimestampInSeconds(sendTime);
    std::string encodedPacket = encodeTimestampAndPayload(timestamp, packetBuffer_);
    client_.send(encodedPacket);
    ++packetsSent_;

    // Compute delay until next send
    const auto delay = pattern_.generateDelay(
        context_.traffic.packetSize,
        context_.traffic.intervalSec);

    // Schedule next send
    nextTime_ += delay;
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto ec) { sendOne(ec); });
}

// Binary encode: double timestamp + payload → string
std::string TrafficScheduler::encodeTimestampAndPayload(const double timestamp, const std::vector<uint8_t> &payload) {
    std::string result;
    result.resize(sizeof(double) + payload.size());

    // write the timestamp as binary (first 8 bytes)
    std::memcpy(&result[0], &timestamp, sizeof(double));

    // append the payload data
    std::memcpy(&result[sizeof(double)], payload.data(), payload.size());

    return result;
}

double TrafficScheduler::getTimestampInSeconds(const std::chrono::system_clock::time_point timePoint) {
    return duration_cast<std::chrono::duration<double> >(timePoint.time_since_epoch()).count();
}
