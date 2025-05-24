// udp_server.cpp
#include "udp_server.h"
#include <iostream>

UdpServer::UdpServer(asio::io_context &io_context,
                     unsigned short port,
                     ReceiveHandler handler)
    : io_context_(io_context),
      strand_(asio::make_strand(io_context_)),
      socket_(io_context_, udp::endpoint(udp::v4(), port)),
      receive_handler_(std::move(handler)), inbound_buff_() {
}

UdpServer::~UdpServer() {
    UdpServer::stop();
}

void UdpServer::start() {
    doReceive();
}

void UdpServer::send(const std::string &message) {
    asio::post(strand_, [self = shared_from_this(), message]() {
        const bool writing = !self->outbound_queue_.empty();
        self->outbound_queue_.push_back(message);
        if (!writing) self->doWrite();
    });
}

void UdpServer::stop() {
    asio::post(strand_, [this]() {
        if (socket_.is_open()) socket_.close();
    });
}

void UdpServer::doReceive() {
    auto self = shared_from_this();
    socket_.async_receive_from(
        asio::buffer(inbound_buff_), remote_endpoint_,
        asio::bind_executor(strand_,
                            [this, self](const ErrorCode &ec, std::size_t bytes_recvd) {
                                if (ec) {
                                    std::cerr << "UDP receive error: " << ec.message() << "\n";
                                    return;
                                }
                                const std::string msg(inbound_buff_.data(), bytes_recvd);
                                receive_handler_(msg);
                                doReceive();
                            }));
}

void UdpServer::doWrite() {
    auto self = shared_from_this();
    const std::string &msg = outbound_queue_.front();
    socket_.async_send_to(
        asio::buffer(msg), remote_endpoint_,
        asio::bind_executor(strand_,
                            [this, self](const ErrorCode &ec, std::size_t /*bytes*/) {
                                if (ec) {
                                    std::cerr << "UDP send error: " << ec.message() << "\n";
                                    return;
                                }
                                outbound_queue_.pop_front();
                                if (!outbound_queue_.empty()) doWrite();
                            }));
}

void UdpServer::setReceiveHandler(ReceiveHandler handler) {
    // store it and hook it up to your async reads…
    receive_handler_ = std::move(handler);
}
