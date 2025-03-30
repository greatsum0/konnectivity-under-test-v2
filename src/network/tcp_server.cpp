#include "tcp_server.h"
#include <iostream>
#include <utility>

TcpServer::TcpServer(ServerContext& context)
    : context_(context),
      acceptor_(context.ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.getPortForProtocol("tcp"))),
      socket_(context.ioContext),
      totalBytesReceived_(0) {}  // initialize the total byte counter to zero

void TcpServer::run() {
    acceptConnection();
}

void TcpServer::acceptConnection() {
    std::cout << "Waiting for connection on port: " << acceptor_.local_endpoint().port() << std::endl;

    acceptor_.async_accept(socket_, [this](const asio::error_code& error) {
        if (!error) {
            std::cout << "Connection accepted from: " << socket_.remote_endpoint() << std::endl;

            totalBytesReceived_ = 0; // reset the total byte counter for the new session

            if (context_.metricCollector) {
                std::cout << "Starting TCP metric collection..." << std::endl;
                context_.metricCollector->start();
            }

            readData();
        } else {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }
    });
}

void TcpServer::readData() {
    socket_.async_read_some(asio::buffer(buffer_),
        [this](const asio::error_code& ec, std::size_t length) {
            if (!ec) {
                totalBytesReceived_ += length;  // add the length of the received data to the total byte counter
                context_.metricCollector->addBytesTransferred(static_cast<int>(length));
                readData();
            } else {
                std::cerr << "Read error: " << ec.message() << std::endl;

                if (context_.metricCollector) {
                    context_.metricCollector->stop();
                }

                std::cout << "[SUMMARY] Total bytes received in session: "
                          << totalBytesReceived_ << " bytes (≈ "
                          << totalBytesReceived_ / 1'000'000.0 << " MB)\n";

                socket_.close();
                this->acceptConnection();
            }
        });
}
