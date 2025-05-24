// udp_server.hpp
#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <array>

#include "inetwork_server.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using ErrorCode = boost::system::error_code;

class UdpServer : public std::enable_shared_from_this<UdpServer>, public INetworkServer {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    UdpServer(asio::io_context &io_context,
              unsigned short port,
              ReceiveHandler handler);

    ~UdpServer() override;

    /// Start receiving datagrams on the bound port
    void start() override;

    /// Queue a datagram to send to the last sender
    void send(const std::string &message) override;

    /// Close the socket
    void stop() override;

    void setReceiveHandler(ReceiveHandler handler) override;

private:
    void doReceive();

    void doWrite();

    asio::io_context &io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    ReceiveHandler receive_handler_;
    std::array<char, 131072> inbound_buff_;
    std::deque<std::string> outbound_queue_;
};
