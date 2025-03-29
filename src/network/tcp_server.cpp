#include "tcp_server.h"
#include <iostream>
#include <utility>

TcpServer::TcpServer(ServerContext& context)
    : context_(context),
      acceptor_(context.ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), context.getPortForProtocol("tcp"))),
      socket_(context.ioContext) {}

void TcpServer::run() {
    acceptConnection();
}

void TcpServer::acceptConnection() {
    std::cout << "Waiting for connection on port: " << acceptor_.local_endpoint().port() << std::endl;

    acceptor_.async_accept(socket_, [this](const asio::error_code& error) {
        if (!error) {
            std::cout << "Connection accepted from: " << socket_.remote_endpoint() << std::endl;

            if (context_.metricCollector) {
                std::cout << "Starting TCP metric collection..." << std::endl;
                context_.metricCollector->start();
                // scheduleMetricCollectionSummary(context_.ioContext, context_.metricCollector, 15.0, "TCP Server");
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
                context_.metricCollector->addBytesTransferred(static_cast<int>(length));
                readData();
            } else {
                std::cerr << "Read error: " << ec.message() << std::endl;
                socket_.close();

                if (context_.metricCollector) {
                    context_.metricCollector->stop();

                    if (auto manager = context_.metricCollector->getManager()) {
                        manager->printTable();
                        // append timestamp to the output file
                        const auto timestamp = std::to_string(std::time(nullptr));
                        manager->saveAsCSV(timestamp + "_" +context_.config.output_path);
                    }
                }
                // clear the metric collector
                context_.metricCollector->reset();
                this->acceptConnection();
            }
        });
}
