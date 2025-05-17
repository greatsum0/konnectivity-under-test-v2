#include "tcp_server.h"
#include <iostream>
#include <cerrno>

TcpServer::TcpServer(asio::io_context &io, const ServerContext &context)
    : INetworkServer(io, context),
      acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.network.port)) {
}

void TcpServer::start() {
    startAccept();
}

void TcpServer::startAccept() {
    // if acceptor is closed, don’t schedule more accepts
    if (!acceptor_.is_open()) return;

    auto newSocket = std::make_shared<asio::ip::tcp::socket>(ioContext_);
    acceptor_.async_accept(*newSocket,
                           [this, newSocket](const asio::error_code &ec) {
                               if (!ec) {
                                   // — happy path or rejection path —
                                   if (!clientConnected_) {
                                       socket_ = newSocket;
                                       clientConnected_ = true;
                                       std::cout << "[SERVER] Client connected from "
                                               << socket_->remote_endpoint() << "\n";
                                       read();
                                   } else {
                                       std::cout << "[SERVER] Rejecting extra client from "
                                               << newSocket->remote_endpoint() << "\n";
                                       std::error_code ignored;
                                       newSocket->shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
                                       newSocket->close(ignored);
                                   }

                                   // only re-arm when accept succeeded
                                   startAccept();
                               } else {
                                   // suppress expected shutdown errors
                                   if (ec != asio::error::operation_aborted && ec.value() != EBADF) {
                                       std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
                                   }
                                   // do NOT call startAccept() here
                               }
                           }
    );
}

void TcpServer::read() {
    if (!socket_ || !socket_->is_open()) return;

    socket_->async_read_some(
        asio::buffer(buffer_),
        [this](const asio::error_code &ec, std::size_t length) {
            if (!ec && length > 0) {
                // process the message...
                read(); // keep reading
            } else {
                std::cout << "[SERVER] Client disconnected: " << ec.message() << "\n";
                std::error_code shutdownEc;
                if (socket_->is_open()) {
                    socket_->shutdown(asio::ip::tcp::socket::shutdown_both, shutdownEc);
                    socket_->close(shutdownEc);
                }
                clientConnected_ = false;
            }
        }
    );
}

void TcpServer::stop() {
    std::error_code ec;
    if (acceptor_.is_open()) {
        acceptor_.cancel(ec);
        acceptor_.close(ec);
    }
    if (socket_ && socket_->is_open()) {
        socket_->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_->close(ec);
    }
    clientConnected_ = false;
}
