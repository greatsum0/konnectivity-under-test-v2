#include "tcp_client.h"
#include <utility>
#include <iostream>

TcpClient::TcpClient(asio::io_context &io_context, std::string host, const int port)
    : io_context_(io_context), socket_(io_context), resolver_(io_context),
      host_(std::move(host)), port_(port), totalBytesSent_(0) {
}

void TcpClient::connect() {
    try {
        const auto endpoints = resolver_.resolve(host_, std::to_string(port_));
        asio::connect(socket_, endpoints);
        std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
    } catch (const std::system_error &e) {
        std::cerr << "[ERROR] Failed to connect to " << host_ << ":" << port_
                << " - " << e.what() << std::endl;
        throw;
    }
}

void TcpClient::sendMessage(const std::string &message) {
    if (!socket_.is_open()) {
        throw std::runtime_error("Socket is not open");
    }
    asio::write(socket_, asio::buffer(message));
    totalBytesSent_ += message.size(); // add the sent bytes to the counter
}

void TcpClient::printSummary() const {
    std::cout << "[SUMMARY] Total bytes sent in session: "
            << totalBytesSent_ << " bytes (≈ "
            << totalBytesSent_ / 1'000'000.0 << " MB)\n";
}
