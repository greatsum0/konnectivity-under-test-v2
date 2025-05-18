#include "tcp_control_channel.h"
#include <yaml_parser.h>
#include <asio/connect.hpp>
#include <asio/write.hpp>

#include "utils/logger.h"

TcpControlChannel::TcpControlChannel(asio::io_context &io_context)
    : io_(io_context),
      socket_(io_),
      resolver_(io_) {
}

void TcpControlChannel::connect(const std::string &host, uint16_t port) {
    host_ = host;
    port_ = port;
    try {
        auto endpoints = resolver_.resolve(host_, std::to_string(port_));
        asio::connect(socket_, endpoints);
        CTRL_LOG_INFO("TcpControlChannel connected to " << host_ << ":" << port_);
    } catch (const std::system_error &e) {
        CTRL_LOG_ERROR("connect to " << host_ << ":" << port_ << " failed: " << e.what());
        throw;
    }
}

void TcpControlChannel::assignSocket(asio::ip::tcp::socket sock) {
    socket_ = std::move(sock);
    CTRL_LOG_INFO("TcpControlChannel socket assigned from accept()");
}

void TcpControlChannel::startReadLoop() {
    if (!socket_.is_open()) return;
    auto self = shared_from_this();
    auto buf = std::make_shared<std::vector<char> >(1024);

    socket_.async_read_some(
        asio::buffer(*buf),
        [self, buf](const std::error_code &ec, std::size_t len) {
            if (!self->socket_.is_open()) return;

            if (!ec && len > 0) {
                std::string msg(buf->data(), len);
                if (self->messageCallback_)
                    self->messageCallback_(msg);
                self->startReadLoop();
            } else {
                CTRL_LOG_WARN("Read error/EOF: " << ec.message());
                self->close();
                if (self->disconnectCallback_)
                    self->disconnectCallback_();
            }
        }
    );
}

void TcpControlChannel::send(const std::string &msg) {
    if (!socket_.is_open()) return;

    bool wasIdle = writeQueue_.empty();
    writeQueue_.push_back(msg);
    if (wasIdle)
        doWrite();
}

void TcpControlChannel::doWrite() {
    if (!socket_.is_open() || writeQueue_.empty()) return;
    auto self = shared_from_this();
    const auto &front = writeQueue_.front();

    asio::async_write(
        socket_, asio::buffer(front),
        [self](const std::error_code &ec, std::size_t /*bytes*/) {
            if (!ec) {
                self->writeQueue_.pop_front();
                if (!self->writeQueue_.empty())
                    self->doWrite();
            } else {
                CTRL_LOG_ERROR("Send failed: " << ec.message());
                self->close();
                if (self->disconnectCallback_)
                    self->disconnectCallback_();
            }
        }
    );
}

void TcpControlChannel::close() {
    if (!socket_.is_open()) return;

    std::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if (ec && ec != asio::error::not_connected) {
        CTRL_LOG_ERROR("shutdown error: " << ec.message());
    }

    socket_.close(ec);
    if (ec && ec != asio::error::not_connected) {
        CTRL_LOG_ERROR("close error: " << ec.message());
    } else {
        CTRL_LOG_INFO("Socket closed cleanly");
    }
}

bool TcpControlChannel::isOpen() const {
    return socket_.is_open();
}

void TcpControlChannel::setMessageCallback(std::function<void(const std::string &)> cb) {
    messageCallback_ = std::move(cb);
}

void TcpControlChannel::setDisconnectCallback(std::function<void()> cb) {
    disconnectCallback_ = std::move(cb);
}

void TcpControlChannel::sendScenarioFile(const ClientContext &context) {
    auto yaml = YamlParser::toYamlString(context);
    send(yaml);
}
