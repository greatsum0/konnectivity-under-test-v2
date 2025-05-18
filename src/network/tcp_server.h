#pragma once

#include "inetwork_server.h"
#include <asio.hpp>
#include <array>
#define SERVER_LOG_INFO(msg) \
LOG_INFO("[TRAFFIC] " << msg)

#define SERVER_LOG_WARN(msg) \
LOG_WARN("[TRAFFIC] " << msg)

#define SERVER_LOG_ERROR(msg) \
LOG_ERROR("[TRAFFIC] " << msg)

class TcpServer final : public INetworkServer {
public:
    TcpServer(asio::io_context &io, const ServerContext &context);

    void start() override;

    void stop() override;

private:
    void startAccept();

    void handleAccept(const std::error_code &ec, std::shared_ptr<asio::ip::tcp::socket> newSocket);

    void read();

    asio::ip::tcp::acceptor acceptor_;
    std::array<char, 17356> buffer_{};
    std::shared_ptr<asio::ip::tcp::socket> socket_; // active client socket
    bool clientConnected_ = false; // true if a client is connected
};
