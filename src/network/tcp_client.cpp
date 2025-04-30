#include "tcp_client.h"
#include <iostream>
#include <asio/connect.hpp>
#include <asio/write.hpp>


TcpClient::TcpClient(asio::io_context &io, const ClientContext &ctx)
    : io_(io),
      socket_(io_),
      resolver_(io_),
      host_(ctx.network.serverAddress),
      portStr_(std::to_string(ctx.network.serverPort)) {
}

void TcpClient::connect() {
    try {
        auto endpoints = resolver_.resolve(host_, portStr_);

        asio::connect(socket_, endpoints);
        std::cout << "[INFO] TCP connected to " << host_ << ":" << portStr_ << "\n";
    } catch (const std::system_error &e) {
        std::cerr << "[ERROR] connect() failed: " << e.what() << "\n";
        throw;
    }
}

void TcpClient::send(const std::vector<uint8_t> &data) {
    try {
        asio::write(socket_, asio::buffer(data));
    } catch (const std::system_error &e) {
        std::cerr << "[ERROR] send() failed: " << e.what() << "\n";
        throw;
    }
}

void TcpClient::disconnect() {
    std::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    if (ec) {
        std::cerr << "[WARN] disconnect() error: " << ec.message() << "\n";
    } else {
        std::cout << "[INFO] TCP disconnected cleanly\n";
    }
}
