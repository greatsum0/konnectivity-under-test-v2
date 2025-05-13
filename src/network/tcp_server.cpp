#include "tcp_server.h"
#include <iostream>

TcpServer::TcpServer(asio::io_context &io, const ServerContext &context)
    : INetworkServer(io, context),
      acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.network.port)) {
}

void TcpServer::start() {
    startAccept();
}

void TcpServer::startAccept() {
    auto newSocket = std::make_shared<asio::ip::tcp::socket>(ioContext_);

    acceptor_.async_accept(*newSocket,
                           [this, newSocket](const asio::error_code &ec) {
                               handleAccept(ec, newSocket);
                           });
}

void TcpServer::handleAccept(const std::error_code &ec, std::shared_ptr<asio::ip::tcp::socket> newSocket) {
    if (!ec) {
        if (!clientConnected_) {
            socket_ = newSocket;
            clientConnected_ = true;

            std::cout << "[SERVER] Client connected from "
                    << socket_->remote_endpoint() << "\n";

            read();
        } else {
            std::cout << "[SERVER] Rejecting extra client from "
                    << newSocket->remote_endpoint() << "\n";

            std::error_code ignoredEc;
            newSocket->shutdown(asio::ip::tcp::socket::shutdown_both, ignoredEc);
            newSocket->close(ignoredEc);
        }
    } else {
        std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
    }

    // always continue accepting for future clients
    startAccept();
}

void TcpServer::read() {
    if (!socket_ || !socket_->is_open()) return;

    socket_->async_read_some(
        asio::buffer(buffer_),
        [this](const asio::error_code &ec, std::size_t length) {
            if (!ec && length > 0) {
                std::string msg(buffer_.data(), length);
                // You can add a callback or processing logic here
                read(); // continue reading
            } else {
                std::cout << "[SERVER] Client disconnected: "
                        << ec.message() << "\n";

                std::error_code shutdownEc;
                socket_->shutdown(asio::ip::tcp::socket::shutdown_both, shutdownEc);
                socket_->close(shutdownEc);
                clientConnected_ = false;
            }
        });
}

void TcpServer::stop() {
    std::error_code ec;

    if (acceptor_.is_open()) {
        acceptor_.cancel(ec);
        if (ec) std::cerr << "[SERVER] acceptor cancel failed: " << ec.message() << "\n";
        acceptor_.close(ec);
        if (ec) std::cerr << "[SERVER] acceptor close failed: " << ec.message() << "\n";
    }

    if (socket_ && socket_->is_open()) {
        socket_->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) std::cerr << "[SERVER] socket shutdown failed: " << ec.message() << "\n";
        socket_->close(ec);
        if (ec) std::cerr << "[SERVER] socket close failed: " << ec.message() << "\n";
    }

    clientConnected_ = false;
}
