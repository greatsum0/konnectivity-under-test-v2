#include "udp_server.h"
#include <iostream>

UdpServer::UdpServer(ServerContext& context)
    : context_(context),
      socket_(context.ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), context.getPortForProtocol("udp"))) {}

void UdpServer::run() {
    if (context_.metricCollector) {
        std::cout << "Starting UDP metric collection..." << std::endl;
        context_.metricCollector->start();
        // scheduleMetricCollectionSummary(context_.ioContext, context_.metricCollector, 15.0, "UDP Server");
    }

    receive();
}

void UdpServer::receive() {
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd) {
            if (!ec && bytes_recvd > 0) {
                if (context_.metricCollector) {
                    context_.metricCollector->addBytesTransferred(static_cast<int>(bytes_recvd));
                }
            } else if (ec) {
                std::cerr << "UDP Receive error: " << ec.message() << std::endl;
            }

            receive(); // Continue receiving
        });
}
