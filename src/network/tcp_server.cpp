#include "tcp_server.h"
#include <iostream>

TcpServer::TcpServer(asio::io_context& io, const ServerContext& context)
    : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.port)),
      socket_(io),
      context_(context) {}

void TcpServer::start() {
    accept();
}

void TcpServer::accept() {
    acceptor_.async_accept(socket_, [this](std::error_code ec) {
        if (!ec) {
            std::cout << "Connection accepted.\n";
        }
        accept();
    });
}
