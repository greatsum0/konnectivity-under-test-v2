#pragma once

#include "i_udp_client.h"        /* ← base class we wrote earlier */

#include <vector>
#include <cstdint>
#include <chrono>
#include <context.h>
#include <itraffic_pattern.h>

using namespace kut::net::udp::client;

class UDPClient final : public client_interface {
public:

    explicit UDPClient(boost::asio::io_context &ioContext,std::size_t pktSize, std::unique_ptr<ITrafficPattern> pattern);

    void start(const std::string &host, uint16_t port);

    void stop();

    /* optional echo */
    void onMessage(std::span<const std::byte> data) override;

private:
    void sendOne(const boost::system::error_code &ec);

    /* same data members */
    std::vector<uint8_t> payload_;
    boost::asio::const_buffer payloadBuf_;
    std::unique_ptr<ITrafficPattern> pattern_;
    boost::asio::steady_timer timer_;
    std::chrono::steady_clock::time_point nextTime_;
};
