#include "tcp_server.h"
#include <iostream>

TcpServer::TcpServer(asio::io_context &io, const ServerContext &context)
    : INetworkServer(io, context),
      acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.port)) {
}

void TcpServer::start() {
    startAccept();
}

void TcpServer::startAccept() {
    auto sock = std::make_shared<asio::ip::tcp::socket>(ioContext_);
    acceptor_.async_accept(*sock,
                           [this, sock](const asio::error_code &ec) {
                               handleAccept(sock, ec);
                           });
}

void TcpServer::handleAccept(std::shared_ptr<asio::ip::tcp::socket> sock,
                             const asio::error_code &ec) {
    if (!ec) {
        std::cout << "[SERVER] Connection accepted from "
                << sock->remote_endpoint() << "\n";
        read(sock);
    } else {
        std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
    }
    startAccept();
}

void TcpServer::read(std::shared_ptr<asio::ip::tcp::socket> sock) {
    sock->async_read_some(
        asio::buffer(buffer_),
        [this, sock](const asio::error_code &ec, std::size_t length) {
            if (!ec) {
                // You can process `length` bytes from buffer_ here.
                read(sock);
            } else {
                std::cout << "[SERVER] Client disconnected: "
                        << ec.message() << "\n";
            }
        });
}
