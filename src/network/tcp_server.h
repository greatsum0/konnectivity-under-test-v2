#pragma once

#include <asio.hpp>
#include <memory>
#include <array>
#include "iserver.h"
#include "server_context.h"

#define BUFFER_SIZE 4096

class TcpServer final : public IServer {
public:
    explicit TcpServer(ServerContext& context);

    void run() override;

private:
    void acceptConnection();
    void readData();

    ServerContext& context_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
    std::array<char, BUFFER_SIZE> buffer_{};
};
