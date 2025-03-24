#pragma once

#include <asio.hpp>

#include "inetwork_client.h"

class UdpClient : public INetworkClient {
public:
    UdpClient(asio::io_context &io_context, const std::string &host, int port);

    void sendMessage(const std::string &message) override;

private:
    asio::io_context &io_context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint endpoint_;
};
