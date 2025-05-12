#pragma once

#include "inetwork_server.h"
#include <asio.hpp>
#include <memory>

class TcpServer final : public INetworkServer {
public:
    TcpServer(asio::io_context &io, const ServerContext &context);

    void start() override;

    void stop() override;

private:
    void startAccept();

    void handleAccept(std::shared_ptr<asio::ip::tcp::socket> sock,
                      const asio::error_code &ec);

    void read(std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::ip::tcp::acceptor acceptor_;
    std::array<char, 8192> buffer_{};
};
