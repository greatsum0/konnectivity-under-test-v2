#pragma once

#include <asio.hpp>
#include <array>
#include <memory>
#include "iserver.h"
#include "server_context.h"

#define BUFFER_SIZE 4096

class UdpServer final : public IServer {
public:
    explicit UdpServer(ServerContext& context);

    void run() override;

private:
    void receive();

    ServerContext& context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remote_endpoint_;
    std::array<char, BUFFER_SIZE> recv_buffer_{};
};
