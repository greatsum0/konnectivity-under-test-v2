// tcp_server.cpp
#include "tcp_server.h"
#include <iostream>
#include <logger.h>

TcpServer::TcpServer(asio::io_context &io_context,
                     const unsigned short port,
                     ReceiveHandler handler)
    : io_context_(io_context),
      strand_(asio::make_strand(io_context_)),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context_),
      receive_handler_(std::move(handler)) {
}

TcpServer::~TcpServer() {
    TcpServer::stop();
}

void TcpServer::start() {
    doAccept();
}

void TcpServer::send(const std::string &message) {
    asio::post(strand_, [self = shared_from_this(), message]() {
        bool writing = !self->outbound_queue_.empty();
        self->outbound_queue_.push_back(message + "\n");
        if (!writing) {
            self->doWrite();
        }
    });
}

void TcpServer::stop() {
    asio::post(strand_, [this]() {
        if (socket_.is_open()) socket_.close();
        if (acceptor_.is_open()) acceptor_.close();
    });
}

void TcpServer::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(socket_,
                           asio::bind_executor(strand_,
                                               [this, self](const ErrorCode &ec) {
                                                   if (ec) {
                                                       std::cerr << "Accept error: " << ec.message() << '\n';
                                                   } else {
                                                       doRead();
                                                   }
                                               }));
}

void TcpServer::doRead() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(inbound_buff_),
        asio::bind_executor(strand_,
                            [this, self](const ErrorCode &ec, std::size_t bytes) {
                                if (ec) {
                                    socket_.close();
                                    return;
                                }

                                // 1) re-arm the next async_read_some immediately
                                doRead();

                                // 2) hand the bytes off to your receive_handler_
                                //    so your callback gets called every time you get data
                                std::string msg(inbound_buff_.data(), bytes);
                                receive_handler_(std::move(msg));
                            }));
}


void TcpServer::doWrite() {
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

void TcpServer::setReceiveHandler(ReceiveHandler handler) {
    // store it and hook it up to your async reads…
    receive_handler_ = std::move(handler);
}

void TcpServer::syncSend(const std::string &message) {
    boost::system::error_code ec;
    auto full = message + "\n";
    // We assume the socket_ is already connected
    std::size_t n = boost::asio::write(socket_,
                                       boost::asio::buffer(full),
                                       ec);
    if (ec) {
        Logger::instance().error("TcpServer::syncSend error: " + ec.message());
    } else {
        Logger::instance().debug("TcpServer::syncSend sent " + std::to_string(n));
    }
}
