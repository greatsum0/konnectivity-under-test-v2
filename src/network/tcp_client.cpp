#include "tcp_client.h"

#include <utility>

TcpClient::TcpClient(asio::io_context &io_context, std::string host, const int port)
    : io_context_(io_context), socket_(io_context), resolver_(io_context), host_(std::move(host)), port_(port) {
}

void TcpClient::connect() {
    const auto endpoints = resolver_.resolve(host_, std::to_string(port_));
    asio::connect(socket_, endpoints);
    std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
}

void TcpClient::sendMessage(const std::string &message) {
    if (!socket_.is_open()) {
        throw std::runtime_error("Socket is not open");
    }
    asio::write(socket_, asio::buffer(message));
}
