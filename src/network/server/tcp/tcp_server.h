#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "inetwork_server.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ErrorCode = boost::system::error_code;

class TcpServer : public std::enable_shared_from_this<TcpServer>, public INetworkServer {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    TcpServer(asio::io_context &io_context,
              unsigned short port,
              ReceiveHandler handler);

    ~TcpServer() override;

    void start() override;

    void send(const std::string &message) override;

    void stop() override;

    void setReceiveHandler(ReceiveHandler handler) override;

    void syncSend(const std::string &message);

private:
    void doAccept();

    void doRead();

    void doWrite();

    asio::io_context &io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    ReceiveHandler receive_handler_;
    std::array<char, 131072> inbound_buff_{};
    std::deque<std::string> outbound_queue_;
};
