#pragma once

#include <asio.hpp>
#include <client_context.h>
#include <functional>
#include <string>

class TcpControlChannel {
public:
    explicit TcpControlChannel(asio::io_context& io_context);

    // Client-side
    void connect(const std::string& host, uint16_t port);

    // Server-side
    void assignSocket(asio::ip::tcp::socket socket);

    void startReadLoop();
    void send(const std::string& msg);
    void close();

    void setMessageCallback(std::function<void(const std::string&)> cb);

    void sendScenarioFile(const ClientContext &context);

private:
    asio::io_context &io_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string portStr_;
    std::function<void(const std::string&)> messageCallback_;
};
