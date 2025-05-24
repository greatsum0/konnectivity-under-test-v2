// udp_client.hpp
#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <array>

#include "inetwork_client.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using ErrorCode = boost::system::error_code;

class UdpClient : public std::enable_shared_from_this<UdpClient>, public INetworkClient {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    UdpClient(asio::io_context &io_context,
              const std::string &host,
              unsigned short port,
              ReceiveHandler handler);

    ~UdpClient() override;

    /// Resolve remote host and start listening for replies
    void start() override;

    /// Queue a datagram to send to the remote server
    void send(const std::string &message) override;

    /// Close the socket and cancel pending resolves
    void stop() override;

    void setReceiveHandler(ReceiveHandler handler) override;

private:
    enum class State { Disconnected, Resolving, Ready };

    void doResolve();

    void doReceive();

    void doWrite();

    asio::io_context &io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    udp::resolver resolver_;
    udp::socket socket_;
    udp::endpoint server_endpoint_;
    std::string host_;
    unsigned short port_;
    ReceiveHandler receive_handler_;
    std::array<char, 1024> recv_buffer_;
    std::deque<std::string> outbound_queue_;
    State state_{State::Disconnected};
};
