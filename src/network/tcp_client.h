#pragma once

#include "inetwork_client.h"
#include "client_context.h"
#include <asio.hpp>
#pragma once

class TcpClient final : public INetworkClient {
public:
    TcpClient(asio::io_context &io, const ClientContext &ctx);

    void connect() override;

    void send(const std::vector<uint8_t> &data) override;

    void disconnect() override;

private:
    asio::io_context &io_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string portStr_;
};
