#pragma once

#include "i_tcp_client.h"

#include <vector>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <itraffic_pattern.h>

class TCPClient final : public kut::net::tcp::client::client_interface {
public:

    explicit TCPClient(boost::asio::io_context &ioContext,std::size_t pktSize, std::unique_ptr<ITrafficPattern> pattern);

    void start(const std::string &host, uint16_t port);

    void stop();

    // Echo back whatever we get (optional)
    void onMessage(std::span<const std::byte> data) override;

private:

    void sendOne(const boost::system::error_code &ec);

    std::vector<uint8_t> payload_;
    boost::asio::const_buffer payloadBuf_;
    std::unique_ptr<ITrafficPattern> pattern_;
    boost::asio::steady_timer timer_;
    std::chrono::steady_clock::time_point nextTime_;
};
