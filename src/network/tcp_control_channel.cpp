#include "tcp_control_channel.h"

#include <client_context.h>
#include <yaml_parser.h>
#include <iostream>

TcpControlChannel::TcpControlChannel(asio::io_context &io_context)
    : io_(io_context),
      socket_(io_),
      resolver_(io_) {
}

void TcpControlChannel::connect(const std::string &host, uint16_t port) {
    try {
        host_ = host;
        portStr_ = std::to_string(port);
        const auto endpoints = resolver_.resolve(host_, portStr_);
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
    auto self = shared_from_this();
    auto buf = std::make_shared<std::vector<char> >(1024);

    socket_.async_read_some(asio::buffer(*buf),
                            [self, buf](const std::error_code ec, const std::size_t len) {
                                if (!self->socket_.is_open()) return;

                                if (!ec && len > 0) {
                                    const std::string msg(buf->data(), len);
                                    if (self->messageCallback_) self->messageCallback_(msg);
                                    self->startReadLoop();
                                } else if (ec == asio::error::eof) {
                                    std::cerr << "[TcpControlChannel] Remote side closed connection (EOF).\n";
                                    self->close();
                                } else if (ec) {
                                    std::cerr << "[TcpControlChannel] Read error: " << ec.message() << "\n";
                                    self->close();
                                }
                            });
}

void TcpControlChannel::send(const std::string &msg) {
    if (!socket_.is_open()) return;

    const bool writeIdle = writeQueue_.empty();
    writeQueue_.push_back(msg);

    if (writeIdle) {
        doWrite();
    }
}

void TcpControlChannel::doWrite() {
    if (!socket_.is_open() || writeQueue_.empty()) return;

    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(writeQueue_.front()),
                      [this, self](const std::error_code ec, std::size_t /*length*/) {
                          if (!ec) {
                              writeQueue_.pop_front();
                              if (!writeQueue_.empty()) {
                                  doWrite();
                              }
                          } else {
                              std::cerr << "[TcpControlChannel] Send failed: " << ec.message() << "\n";
                              self->close();
                          }
                      });
}

void TcpControlChannel::close() {
    if (socket_.is_open()) {
        std::error_code ec;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            std::cerr << "[TcpControlChannel] Shutdown error: " << ec.message() << "\n";
        }

        socket_.close(ec);
        if (ec) {
            std::cerr << "[TcpControlChannel] Close error: " << ec.message() << "\n";
        } else {
            std::cout << "[TcpControlChannel] Socket closed cleanly.\n";
        }
    }
}

void TcpControlChannel::setMessageCallback(std::function<void(const std::string &)> cb) {
    messageCallback_ = std::move(cb);
}

void TcpControlChannel::sendScenarioFile(const ClientContext &context) {
    const std::string yamlText = YamlParser::toYamlString(context);
    send(yamlText);
}
