#pragma once

#include "inetwork_server.h"
#include <asio.hpp>

#define SERVER_LOG_INFO(msg) \
LOG_INFO("[TRAFFIC] " << msg)

#define SERVER_LOG_WARN(msg) \
LOG_WARN("[TRAFFIC] " << msg)

#define SERVER_LOG_ERROR(msg) \
LOG_ERROR("[TRAFFIC] " << msg)

class UdpServer final : public INetworkServer {
public:
    UdpServer(asio::io_context &io, const ServerContext &context);

    void start() override;

    void stop() override;

private:
    void receive();

    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remoteEndpoint_;
    std::array<char, 17356> buffer_{};
};
