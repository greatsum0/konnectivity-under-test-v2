// tcp_client.cpp
#include "tcp_client.h"
#include <iostream>
#include <logger.h>

TcpClient::TcpClient(asio::io_context &io_context,
                     const std::string &host,
                     unsigned short port,
                     ReceiveHandler handler)
    : io_context_(io_context),
      strand_(asio::make_strand(io_context_)),
      resolver_(io_context_),
      socket_(io_context_),
      host_(host),
      port_(port),
      receive_handler_(std::move(handler)) {
}

TcpClient::~TcpClient() {
    TcpClient::stop();
}

void TcpClient::start() {
    if (state_ != State::Disconnected) return;
    state_ = State::Resolving;
    doResolve();
}

void TcpClient::send(const std::string &message) {
    asio::post(strand_, [self = shared_from_this(), message]() {
        const bool wasEmpty = self->outbound_queue_.empty();
        self->outbound_queue_.push_back(message + "\n");
        if (self->state_ == State::Connected && wasEmpty) {
            self->doWrite();
        }
    });
}

void TcpClient::stop() {
    asio::post(strand_, [this]() {
        if (socket_.is_open()) socket_.close();
        resolver_.cancel();
        state_ = State::Disconnected;
    });
}

void TcpClient::setReceiveHandler(ReceiveHandler handler) {
    receive_handler_ = std::move(handler);
}

void TcpClient::doResolve() {
    auto self = shared_from_this();
    resolver_.async_resolve(host_, std::to_string(port_),
                            asio::bind_executor(strand_,
                                                [this, self](const ErrorCode &ec, auto results) {
                                                    if (ec) {
                                                        std::cerr << "Resolve error: " << ec.message() << "\n";
                                                        state_ = State::Disconnected;
                                                    } else {
                                                        state_ = State::Connecting;
                                                        doConnect(results);
                                                    }
                                                }));
}

void TcpClient::doConnect(const tcp::resolver::results_type &endpoints) {
    auto self = shared_from_this();
    asio::async_connect(socket_, endpoints,
                        asio::bind_executor(strand_,
                                            [this, self](const ErrorCode &ec, const tcp::endpoint &) {
                                                if (ec) {
                                                    Logger::instance().error("Connect error: " + ec.message());
                                                    socket_.close();
                                                    state_ = State::Disconnected;
                                                } else {
                                                    boost::asio::ip::tcp::no_delay noDelayOption(true);
                                                    boost::system::error_code ec2;
                                                    socket_.set_option(noDelayOption, ec2);
                                                    if (ec2) {
                                                        Logger::instance().warning(
                                                            "Failed to set TCP_NODELAY: " + ec2.message());
                                                    }

                                                    state_ = State::Connected;
                                                    doRead();

                                                    // flush any queued writes now that we're actually connected
                                                    if (!outbound_queue_.empty()) {
                                                        doWrite();
                                                    }
                                                }
                                            }));
}


void TcpClient::doRead() {
    auto self = shared_from_this();
    asio::async_read_until(socket_, asio::dynamic_buffer(inbound_buffer_), '\n',
                           asio::bind_executor(strand_,
                                               [this, self](const ErrorCode &ec, std::size_t bytes) {
                                                   if (ec) {
                                                       std::cerr << "Read error: " << ec.message() << '\n';
                                                       socket_.close();
                                                   } else {
                                                       std::string msg(inbound_buffer_.substr(0, bytes - 1));
                                                       inbound_buffer_.erase(0, bytes);
                                                       receive_handler_(msg);
                                                       doRead();
                                                   }
                                               }));
}

void TcpClient::doWrite() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(outbound_queue_.front()),
                      asio::bind_executor(strand_,
                                          [this, self](const ErrorCode &ec, std::size_t /*bytes*/) {
                                              if (ec) {
                                                  std::cerr << "Write error: " << ec.message() << '\n';
                                                  socket_.close();
                                              } else {
                                                  outbound_queue_.pop_front();
                                                  if (!outbound_queue_.empty()) {
                                                      doWrite();
                                                  }
                                              }
                                          }));
}

void TcpClient::syncSend(const std::string &message) {
    boost::system::error_code ec;
    auto full = message + "\n";
    std::size_t n = boost::asio::write(socket_,
                                       boost::asio::buffer(message),
                                       ec);
    if (ec) {
        Logger::instance().error("TcpClient::syncSend error: " + ec.message());
    } else {
        Logger::instance().debug(
            "TcpClient::syncSend sent " + std::to_string(n));
    }
}
