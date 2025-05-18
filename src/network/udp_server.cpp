#include "udp_server.h"
#include <iostream>

#include "utils/logger.h"

UdpServer::UdpServer(asio::io_context &io, const ServerContext &context)
    : INetworkServer(io, context),
      socket_(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), context.network.port)) {
}

void UdpServer::start() {
    receive();
}

void UdpServer::receive() {
    socket_.async_receive_from(
        asio::buffer(buffer_), remoteEndpoint_,
        [this](const std::error_code ec, const std::size_t bytesReceived) {
            if (!ec) {
                // is this our end-of-traffic marker?
                if (bytesReceived == 3 &&
                    std::string(buffer_.data(), buffer_.data() + 3) == "END") {
                    SERVER_LOG_INFO("End-of-traffic marker received\n");
                    ioContext_.stop();
                } else {
                    receive();
                }
            } else {
                SERVER_LOG_INFO("UDP receive error: "
                    << ec.message() << "\n");
                ioContext_.stop();
            }
        });
}

void UdpServer::stop() {
    std::error_code ec;
    socket_.cancel(ec);
    socket_.close(ec);
    SERVER_LOG_INFO("Stopped.\n");
}
