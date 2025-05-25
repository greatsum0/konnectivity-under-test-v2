// tcp_client.hpp
#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "inetwork_client.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ErrorCode = boost::system::error_code;

class TcpClient : public std::enable_shared_from_this<TcpClient>, public INetworkClient {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    TcpClient(asio::io_context &io_context,
              const std::string &host,
              unsigned short port,
              ReceiveHandler handler);

    ~TcpClient() override;

    void start() override;

    void send(const std::string &message) override;

    void stop() override;

    void setReceiveHandler(ReceiveHandler handler) override;

    void syncSend(const std::string &message);

private:
    enum class State {
        Disconnected,
        Resolving,
        Connecting,
        Connected
    };

    void doResolve();

    void doConnect(const tcp::resolver::results_type &endpoints);

    void doRead();

    void doWrite();

    asio::io_context &io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string host_;
    unsigned short port_;
    ReceiveHandler receive_handler_;
    std::string inbound_buffer_;
    std::deque<std::string> outbound_queue_;
    State state_{State::Disconnected};
};
