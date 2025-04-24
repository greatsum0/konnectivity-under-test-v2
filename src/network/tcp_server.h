#pragma once

#include "server_context.h"
#include <asio.hpp>
#include <memory>

class TcpServer {
public:
    TcpServer(asio::io_context &io, const ServerContext &context);

    void start();

private:
    void startAccept();

    void handleAccept(std::shared_ptr<asio::ip::tcp::socket> sock,
                      const asio::error_code &ec);

    void read(std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::io_context &ioContext_;
    asio::ip::tcp::acceptor acceptor_;
    const ServerContext &context_;
    std::array<char, 8192> buffer_{};
};
