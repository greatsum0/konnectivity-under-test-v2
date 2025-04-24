#pragma once

#include "server_context.h"
#include <asio.hpp>
#include <memory>

class TcpServer {
public:
    TcpServer(asio::io_context& io, const ServerContext& context);
    void start();

private:
    void accept();

    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
    const ServerContext& context_;
};
