#include "udp_client.h"
#include <iostream>

using boost::asio::ip::udp;
using ErrorCode = boost::system::error_code;

UdpClient::UdpClient(boost::asio::io_context &io_context,
                     const std::string &host,
                     unsigned short port,
                     ReceiveHandler handler)
    : io_context_(io_context),
      strand_(boost::asio::make_strand(io_context_)),
      resolver_(io_context_),
      socket_(io_context_),
      host_(host),
      port_(port),
      receive_handler_(std::move(handler)), recv_buffer_() {
}

UdpClient::~UdpClient() {
    UdpClient::stop();
}

void UdpClient::start() {
    if (state_ != State::Disconnected) return;
    state_ = State::Resolving;
    doResolve();
}

void UdpClient::send(const std::string &message) {
    asio::post(strand_, [self = shared_from_this(), message]() {
        bool wasEmpty = self->outbound_queue_.empty();
        self->outbound_queue_.push_back(message);
        // Only flush if we're already Ready
        if (self->state_ == State::Ready && wasEmpty) {
            self->doWrite();
        }
    });
}

void UdpClient::stop() {
    asio::post(strand_, [this]() {
        if (socket_.is_open()) socket_.close();
        resolver_.cancel();
        state_ = State::Disconnected;
    });
}

void UdpClient::setReceiveHandler(ReceiveHandler handler) {
    receive_handler_ = std::move(handler);
}

void UdpClient::doResolve() {
    auto self = shared_from_this();
    resolver_.async_resolve(host_, std::to_string(port_),
                            asio::bind_executor(strand_,
                                                [this, self](const ErrorCode &ec, udp::resolver::results_type results) {
                                                    if (ec) {
                                                        std::cerr << "UDP resolve error: " << ec.message() << "\n";
                                                        state_ = State::Disconnected;
                                                    } else {
                                                        // Pick the first endpoint and open the socket
                                                        server_endpoint_ = *results.begin();
                                                        socket_.open(udp::v4());
                                                        state_ = State::Ready;

                                                        // Begin receive loop
                                                        doReceive();

                                                        // Flush any queued writes
                                                        if (!outbound_queue_.empty()) {
                                                            doWrite();
                                                        }
                                                    }
                                                }));
}

void UdpClient::doReceive() {
    auto self = shared_from_this();
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), server_endpoint_,
        asio::bind_executor(strand_,
                            [this, self](const ErrorCode &ec, std::size_t bytes_recvd) {
                                if (ec) {
                                    std::cerr << "UDP receive error: " << ec.message() << "\n";
                                } else {
                                    std::string msg(recv_buffer_.data(), bytes_recvd);
                                    receive_handler_(msg);
                                    doReceive();
                                }
                            }));
}

void UdpClient::doWrite() {
    auto self = shared_from_this();
    socket_.async_send_to(
        asio::buffer(outbound_queue_.front()), server_endpoint_,
        asio::bind_executor(strand_,
                            [this, self](const ErrorCode &ec, std::size_t /*bytes*/) {
                                if (ec) {
                                    std::cerr << "UDP send error: " << ec.message() << "\n";
                                } else {
                                    outbound_queue_.pop_front();
                                    if (!outbound_queue_.empty()) {
                                        doWrite();
                                    }
                                }
                            }));
}
