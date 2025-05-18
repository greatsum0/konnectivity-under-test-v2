#include "tcp_client.h"
#include <asio/connect.hpp>
#include <asio/ip/address.hpp>
#include <asio/write.hpp>
#include <iostream>

#include "utils/logger.h"

#include "utils/get_ip_interface.h"

TcpClient::TcpClient(asio::io_context &io, const ClientContext &ctx)
    : io_(io),
      socket_(io_),
      resolver_(io_),
      host_(ctx.network.serverAddress),
      portStr_(std::to_string(ctx.network.serverPort)),
      context_(ctx) {}

void TcpClient::connect() {
    try {
        std::string ip = get_ip_of_interface(context_.network.interface);
        if (!ip.empty()) {
            socket_.open(asio::ip::tcp::v4());
            const asio::ip::tcp::endpoint local_endpoint(asio::ip::make_address(ip), 0);
            socket_.bind(local_endpoint);
            CLIENT_LOG_INFO("TCP bound to interface IP " << ip);
        } else {
            CLIENT_LOG_WARN("TCP bind skipped (no IP found or non-Linux platform)");
        }

        const auto endpoints = resolver_.resolve(host_, portStr_);
        asio::connect(socket_, endpoints);
        CLIENT_LOG_INFO("TCP connected to " << host_ << ":" << portStr_);
    } catch (const std::system_error &e) {
        CLIENT_LOG_ERROR("[ERROR] connect() failed: " << e.what() << "\n");

        std::error_code ec;
        socket_.close(ec);
        throw;
    }
}

void TcpClient::send(const std::vector<uint8_t> &data) {
    try {
        std::size_t totalBytes = 0;
        while (totalBytes < data.size()) {
            totalBytes += asio::write(
                socket_,
                asio::buffer(data.data() + totalBytes, data.size() - totalBytes)
            );
        }
    } catch (const std::system_error &e) {
        CLIENT_LOG_ERROR("[ERROR] send() failed: " << e.what() << "\n");
        throw;
    }
}

void TcpClient::disconnect() {
    std::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    if (ec && ec != asio::error::not_connected) {
        CLIENT_LOG_WARN("[WARN] close error: " << ec.message() << "\n");
    } else {
        CLIENT_LOG_INFO("TCP disconnected cleanly");
    }
}

bool TcpClient::isConnected() const {
    return socket_.is_open();
}
