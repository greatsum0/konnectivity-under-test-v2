#pragma once

#include "inetwork_server.h"
#include <asio.hpp>
#include <memory>
#include <array>

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
