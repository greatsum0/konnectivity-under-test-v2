#include "udp_server.h"
#include <iostream>

UdpServer::UdpServer(asio::io_context& io, const ServerContext& context)
    : socket_(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), context.port)),
      context_(context) {}

void UdpServer::start() {
    receive();
}

void UdpServer::receive() {
    socket_.async_receive_from(
        asio::buffer(buffer_), remoteEndpoint_,
        [this](std::error_code ec, std::size_t bytesReceived) {
            if (!ec) {
                std::cout << "Received " << bytesReceived << " bytes from "
                          << remoteEndpoint_.address().to_string() << ":"
                          << remoteEndpoint_.port() << "\n";
            }
            receive();
        });
}
