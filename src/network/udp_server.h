#pragma once

#include "inetwork_server.h"
#include <asio.hpp>

class UdpServer final : public INetworkServer {
public:
    UdpServer(asio::io_context &io, const ServerContext &context);

    void start() override;

private:
    void receive();

    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remoteEndpoint_;
    std::array<char, 2048> buffer_{};
};
