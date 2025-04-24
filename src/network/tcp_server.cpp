#include "tcp_server.h"
#include <iostream>

TcpServer::TcpServer(asio::io_context &io, const ServerContext &context)
    : ioContext_(io),
      acceptor_(ioContext_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.port)),
      context_(context) {
}

void TcpServer::start() {
    startAccept();
}

void TcpServer::startAccept() {
    // ioContext_ üzerinden yeni socket yarat
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
    // Her durumda bir sonraki kabulü planla
    startAccept();
}

void TcpServer::read(std::shared_ptr<asio::ip::tcp::socket> sock) {
    sock->async_read_some(
        asio::buffer(buffer_),
        [this, sock](const asio::error_code &ec, std::size_t length) {
            if (!ec) {
                // length kadar bayt okundu, işleyebilirsin...
                read(sock); // aynı soketi okumaya devam et
            } else {
                std::cout << "[SERVER] Client disconnected: "
                        << ec.message() << "\n";
            }
        });
}
