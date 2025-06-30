#pragma once

#include <vector>
#include <cstdint>
#include "i_udp_server.h"

class UDPServer final : public kut::net::udp::server::server_interface {
public:
    explicit UDPServer(boost::asio::io_context &ioContext, uint16_t port);

protected:
    void OnMessage(std::span<const std::byte> msg) override;

    bool OnClientConnect() override;

    void OnClientDisconnect() override;

public:
    void setDoneCallback(std::function<void()> done) override;

private:
    std::function<void()> done_;
};
