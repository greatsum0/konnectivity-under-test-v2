#include "udp_client.h"
#include <iostream>
#include <string>

UdpClient::UdpClient(asio::io_context &io_context, const std::string &host, const int port)
    : io_context_(io_context),
      socket_(io_context, asio::ip::udp::v4()),
      endpoint_(asio::ip::make_address(host), port) {
}

void UdpClient::sendMessage(const std::string &message) {
    asio::error_code ec;
    socket_.send_to(asio::buffer(message), endpoint_, 0, ec);
    if (ec) {
        std::cerr << "UDP send error: " << ec.message() << std::endl;
    }
}
