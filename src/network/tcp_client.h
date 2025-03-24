#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>

#include "inetwork_client.h"

class TcpClient : public INetworkClient {
public:
    TcpClient(asio::io_context &io_context, std::string host, int port);

    void connect();

    void sendMessage(const std::string &message) override;

private:
    asio::io_context &io_context_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    int port_;
};
