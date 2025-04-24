#pragma once

#include "server_context.h"
#include <asio.hpp>

class UdpServer {
public:
    UdpServer(asio::io_context &io, const ServerContext &context);

    void start();

private:
    void receive();

    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remoteEndpoint_;
    std::array<char, 2048> buffer_{};
    const ServerContext &context_;
};
