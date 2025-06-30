#pragma once

#include <vector>
#include <cstdint>
#include "i_tcp_server.h"

class TCPServer final : public kut::net::tcp::server::server_interface {
public:
    explicit TCPServer(boost::asio::io_context &ioContext, uint16_t port);

protected:
    void OnMessage(std::span<const std::byte> msg) override;

    bool OnClientConnect() override;

    void OnClientDisconnect() override;

public:
    void setDoneCallback(std::function<void()> done) override;

private:
    std::function<void()> done_;
};
