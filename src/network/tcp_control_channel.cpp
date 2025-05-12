#include "tcp_control_channel.h"

#include <client_context.h>
#include <iostream>
#include <yaml_parser.h>

TcpControlChannel::TcpControlChannel(asio::io_context &io_context)
    : io_(io_context),
      socket_(io_),
      resolver_(io_) {
}

void TcpControlChannel::connect(const std::string &host, uint16_t port) {
    try {
        this->host_ = host;
        this->portStr_ = std::to_string(port);
        auto endpoints = resolver_.resolve(host_, portStr_);
        asio::connect(socket_, endpoints);
        std::cout << "[INFO] TCP connected to " << host << ":" << port << "\n";
    } catch (const std::system_error &e) {
        std::cerr << "[ERROR] connect() failed: " << e.what() << "\n";
        throw;
    }
}

void TcpControlChannel::assignSocket(asio::ip::tcp::socket socket) {
    socket_ = std::move(socket);
    std::cout << "[TcpControlChannel] Socket assigned from accept().\n";
}

void TcpControlChannel::startReadLoop() {
    if (!socket_.is_open()) return;
    auto buf = std::make_shared<std::vector<char> >(1024);
    socket_.async_read_some(asio::buffer(*buf), [this, buf](std::error_code ec, std::size_t len) {
        if (!ec) {
            std::string msg(buf->data(), len);
            if (messageCallback_) messageCallback_(msg);
            startReadLoop();
        } else {
            std::cerr << "[TcpControlChannel] Read error: " << ec.message() << "\n";
            socket_.close();
        }
    });
}

void TcpControlChannel::send(const std::string &msg) {
    asio::async_write(socket_, asio::buffer(msg), [](std::error_code ec, std::size_t) {
        if (ec) std::cerr << "[TcpControlChannel] Send failed: " << ec.message() << "\n";
    });
}

void TcpControlChannel::close() {
    asio::post(io_, [this]() {
        if (socket_.is_open()) {
            std::error_code ec;
            socket_.cancel(ec);
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);
            std::cout << "[TcpControlChannel] Socket closed.\n";
        }
    });
}

void TcpControlChannel::setMessageCallback(std::function<void(const std::string &)> cb) {
    messageCallback_ = std::move(cb);
}

void TcpControlChannel::sendScenarioFile(const ClientContext &context) {
    std::string yamlText = YamlParser::toYamlString(context);
    asio::write(socket_, asio::buffer(yamlText));
}
