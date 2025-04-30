#pragma once

#include "inetwork_client.h"
#include "client_context.h"
#include <asio.hpp>
#include <string>
#include <vector>

class UdpClient final : public INetworkClient {
public:
    UdpClient(asio::io_context& io, const ClientContext& ctx);

    void connect() override;
    void send(const std::vector<uint8_t>& data) override;
    void disconnect() override;

private:
    asio::io_context& io_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::resolver resolver_;
    asio::ip::udp::endpoint endpoint_;
    std::string host_;
    std::string portStr_;
};
