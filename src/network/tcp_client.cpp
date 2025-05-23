#include "tcp_client.h"
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <iostream>

#include "utils/logger.h"

TcpClient::TcpClient(asio::io_context &io, const ClientContext &ctx)
    : io_(io),
      socket_(io_),
      resolver_(io_),
      host_(ctx.network.serverAddress),
      portStr_(std::to_string(ctx.network.serverPort)) {
}

void TcpClient::connect() {
    // Asynchronously resolve DNS
    resolver_.async_resolve(host_, portStr_,
        [this](const asio::error_code &ec,
               asio::ip::tcp::resolver::results_type results) {
            if (ec) {
                CLIENT_LOG_ERROR("[ERROR] resolve() failed: " << ec.message());
                return;
            }

            // Asynchronously connect to one of the endpoints
            asio::async_connect(socket_, results,
                [this](const asio::error_code &ec,
                       const asio::ip::tcp::endpoint & /*endpoint*/) {
                    if (ec) {
                        CLIENT_LOG_ERROR("[ERROR] connect() failed: " << ec.message());
                    } else {
                        CLIENT_LOG_INFO("TCP connected to " << host_ << ":" << portStr_);
                    }
                });
        });
}

void TcpClient::send(const std::vector<uint8_t> &data) {
    // Asynchronously write the entire buffer
    asio::async_write(socket_,
                      asio::buffer(data.data(), data.size()),
        [this](const asio::error_code &ec, std::size_t bytesTransferred) {
            if (ec) {
                CLIENT_LOG_ERROR("[ERROR] send() failed: " << ec.message());
            }
        });
}

void TcpClient::disconnect() {
    std::error_code ec;

    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if (ec && ec != asio::error::not_connected) {
        CLIENT_LOG_WARN("[WARN] shutdown error: " << ec.message());
    }

    socket_.close(ec);
    if (ec && ec != asio::error::not_connected) {
        CLIENT_LOG_WARN("[WARN] close error: " << ec.message());
    } else {
        CLIENT_LOG_INFO("TCP disconnected cleanly");
    }
}

bool TcpClient::isConnected() const {
    return socket_.is_open();
}
