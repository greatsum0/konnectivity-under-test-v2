#include "udp_client.h"
#include "utils/logger.h"
#include "utils/get_ip_interface.h"
#include <iostream>

#include <asio/ip/address.hpp>

UdpClient::UdpClient(asio::io_context &io, const ClientContext &ctx)
    : io_(io),
      socket_(io_),
      resolver_(io_),
      host_(ctx.network.serverAddress),
      portStr_(std::to_string(ctx.network.serverPort)),
      context_(ctx) {
}

void UdpClient::connect() {
    try {
        const auto results = resolver_.resolve(asio::ip::udp::v4(), host_, portStr_);
        endpoint_ = *results.begin();

        std::string ip = get_ip_of_interface(context_.network.interface);
        if (!ip.empty()) {
            socket_.open(asio::ip::udp::v4());
            const asio::ip::address local_addr = asio::ip::make_address(ip);
            const asio::ip::udp::endpoint local_endpoint(local_addr, 0);
            socket_.bind(local_endpoint);
            CLIENT_LOG_INFO("UDP bound to interface IP " << ip);
        } else {
            CLIENT_LOG_WARN("UDP bind skipped (no IP found or non-Linux platform)");
            socket_.open(asio::ip::udp::v4());
        }
        socket_.connect(endpoint_);

        CLIENT_LOG_INFO("UDP socket connected to "
            << host_ << ":" << portStr_ << "\n");
    } catch (const std::system_error &e) {
        CLIENT_LOG_ERROR("UDP connect() failed: " << e.what() << "\n");
        throw;
    }
}

void UdpClient::send(const std::vector<uint8_t> &data) {
    try {
        socket_.send(asio::buffer(data));
    } catch (const std::system_error &e) {
        CLIENT_LOG_ERROR("UDP send() failed: " << e.what() << "\n");
        throw;
    }
}

void UdpClient::disconnect() {
    std::error_code ec;
    socket_.shutdown(asio::ip::udp::socket::shutdown_both, ec);
    socket_.close(ec);
    if (ec) {
        CLIENT_LOG_WARN("UDP disconnect error: " << ec.message() << "\n");
    } else {
        CLIENT_LOG_INFO("UDP socket closed cleanly\n");
    }
}
