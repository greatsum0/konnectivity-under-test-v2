#include "udp_server.h"

#include <iostream>
#include <cstring>
#include <logger.h>

UDPServer::UDPServer(boost::asio::io_context &ioContext, uint16_t port)
    : server_interface(ioContext, port) {
}

void UDPServer::OnMessage(std::span<const std::byte> msg) {
    // std::cout << "Received (" << msg.size() << " bytes): ";
    // std::cout.write(reinterpret_cast<const char*>(msg.data()), msg.size());
    // std::cout << '\n';
}

bool UDPServer::OnClientConnect() {
    Logger::instance().info("Client accepted.");
    return true; // accept first (and only) client
}

void UDPServer::OnClientDisconnect() {
    Logger::instance().info("Client disconnected.");
    if (done_) done_();
}

void UDPServer::setDoneCallback(std::function<void()> done) {
    done_ = std::move(done);
}
