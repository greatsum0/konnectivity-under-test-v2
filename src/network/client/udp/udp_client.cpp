#include "udp_client.h"

#include <burst_traffic_pattern.h>
#include <iostream>
#include "traffic_pattern_factory.h"

static inline uint64_t toNetwork64(uint64_t value) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(value);
#else
    return value;
#endif
}
UDPClient::UDPClient(boost::asio::io_context &ioContext, const std::size_t pktSize,
                     std::unique_ptr<ITrafficPattern> &pattern)
    : client_interface(ioContext), payload_(pktSize, 0x54),
      payloadBuf_(payload_.data(), payload_.size()),
      pattern_(pattern),
      timer_(ioContext_) {
}

void UDPClient::start(const std::string &host, uint16_t port) {
    if (!connect(host, port)) return;

    /* wait until socket.connect() inside connection_ finishes */
    while (!isConnected())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    /* arm the periodic sender on the io_context thread */
    nextTime_ = std::chrono::steady_clock::now();
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto ec) { sendOne(ec); });
}

void UDPClient::stop() {
    timer_.cancel(); // stop asynchronous loop
    disconnect(); // base-class cleanup + join
}

/* debug/echo callback */
void UDPClient::onMessage(std::span<const std::byte> /*data*/) {
    // std::cout.write(reinterpret_cast<const char*>(data.data()), data.size());
    // std::cout << '\n';
}

void UDPClient::sendOne(const boost::system::error_code &ec) {
    if (ec || !isConnected())
        return; // cancelled / disconnected

    /* ── 1.  Write a 64-bit timestamp into the first 8 bytes ─────────── */
    uint64_t tsNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    uint64_t netTsNs = toNetwork64(tsNs);  // convert to network byte order
    std::memcpy(payload_.data(), &netTsNs, sizeof(netTsNs));


    /* ── 2.  Zero-copy send (payloadBuf_ already views payload_) ─────── */
    send(payloadBuf_);

    /* ── 3.  Schedule next packet (unchanged) ─────────────────────────── */
    const std::size_t pktSize = payload_.size();
    const auto delay = pattern_->generateDelay(pktSize, 1);
    nextTime_ += delay;
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](auto err) { sendOne(err); });
}
