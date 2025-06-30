#include "tcp_server.h"

#include <iostream>
#include <cstring>
#include <logger.h>

TCPServer::TCPServer(boost::asio::io_context &ioContext,uint16_t port)
    : server_interface(ioContext, port) {
}

void TCPServer::OnMessage(std::span<const std::byte> msg)
{
    // std::cout << "Received (" << msg.size() << " bytes): ";
    // std::cout.write(reinterpret_cast<const char*>(msg.data()), msg.size());
    // std::cout << '\n';
}

bool TCPServer::OnClientConnect() {
    Logger::instance().info("Client accepted.");
    return true; // accept first (and only) client
}

void TCPServer::OnClientDisconnect() {
    Logger::instance().info("Client disconnected.");
    if (done_) done_();
}

void TCPServer::setDoneCallback(std::function<void()> done) {
    done_ = std::move(done);
}