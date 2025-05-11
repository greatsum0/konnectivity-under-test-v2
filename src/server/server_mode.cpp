#include "server_mode.h"
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <yaml_parser.h>

#include "cpu_usage_monitor.h"
#include "metric_collector_factory.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "tcp_control_channel.h"
#include "udp_server.h"
#include "tcp_server.h"
#include "utils/csv_writer.h"
#include "utils/pretty_printer.h"

#define CONTROL_PORT 6070

ServerMode::ServerMode(ServerContext context)
    : context(std::move(context)), clientContext() {
}

std::unique_ptr<INetworkServer> ServerMode::createNetworkServer(asio::io_context &io) {
    if (context.network.protocol == "tcp") {
        return std::make_unique<TcpServer>(io, context);
    }
    if (context.network.protocol == "udp") {
        return std::make_unique<UdpServer>(io, context);
    }
    throw std::runtime_error("Unsupported protocol: " + context.network.protocol);
}


std::vector<Metric> ServerMode::combineMetrics(std::vector<Metric> cpuMetrics,
                                               std::vector<Metric> networkMetrics) {
    std::vector<Metric> allMetrics;

    /// append all metrics to a single vector
    /// append network metrics first
    allMetrics.reserve(networkMetrics.size() + cpuMetrics.size());
    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(networkMetrics.begin()),
                      std::make_move_iterator(networkMetrics.end()));

    /// clear network metrics to avoid bloating the memory
    networkMetrics.clear();

    /// append cpu metrics
    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(cpuMetrics.begin()),
                      std::make_move_iterator(cpuMetrics.end()));
    /// clear cpu metrics to avoid bloating the memory
    cpuMetrics.clear();

    return allMetrics;
}


void ServerMode::run() {
    const auto ioTcpController = std::make_shared<asio::io_context>();
    asio::ip::tcp::acceptor acceptor(*ioTcpController, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONTROL_PORT));

    acceptor.async_accept([&](std::error_code ec, asio::ip::tcp::socket socket) {
        if (ec) {
            std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
            return;
        }

        std::cout << "[SERVER] Client connected.\n";

        // create TcpControlChannel and assign the socket
        auto controlChannel = std::make_shared<TcpControlChannel>(*ioTcpController);
        controlChannel->assignSocket(std::move(socket));

        // define the message callback
        controlChannel->setMessageCallback([controlChannel, this](const std::string &msg) {
            if (msg.find("scenario:") != std::string::npos) {
                std::cout << "[SERVER] Scenario received, sending START_SCENARIO\n";
                // Extract the scenario YAML from the message
                std::vector<Metric> cpuMetrics;
                std::vector<Metric> networkMetrics;

                clientContext = YamlParser::parseClientContextFromString(msg);

                PortTrafficMonitor networkMonitor(
                    context.measurement.interface,
                    context.network.port,
                    context.measurement.intervalSec
                );
                CPUUsageMonitor cpuMonitor(
                    context.measurement.intervalSec
                );
                // ----------------------------------------------------
                // 2) network callback
                auto network_collectors = MetricCollectorFactory::create(context.measurement, networkMetrics);
                networkMonitor.start(
                    [&](auto in, auto out) {
                        for (auto &c: network_collectors)
                            c->onNetworkSample(std::chrono::system_clock::now(), in, out);
                    }
                );

                // 3) CPU callback
                auto cpu_collectors = MetricCollectorFactory::create(context.measurement, cpuMetrics);
                cpuMonitor.start(
                    [&](const std::vector<double> &perCoreUsages,
                        const double totalCore,
                        const double selfCpu,
                        const double selfShareTotal,
                        const double selfShareCap) {
                        const auto now = std::chrono::system_clock::now();

                        for (const auto &c: cpu_collectors) {
                            c->onCpuSample(now, totalCore, selfCpu);
                            c->onCpuDetailSample(now, perCoreUsages,
                                                 selfShareTotal, selfShareCap);
                        }
                    }
                );
                //start the server mode
                const auto ioTraffic = std::make_shared<asio::io_context>();
                const auto baseServer = createNetworkServer(*ioTraffic);
                baseServer->start();
                controlChannel->send("START_SCENARIO\n");
                ioTraffic->run();

                networkMonitor.stop();
                cpuMonitor.stop();

                std::vector<Metric> allMetrics = combineMetrics(cpuMetrics, networkMetrics);

                utils::PrettyPrinter::printPivotTable(allMetrics);

                /// create directories if they do not exist
                std::filesystem::create_directories(context.outputPath);

                /// append the filename
                auto now = std::chrono::system_clock::now();
                auto now_c = std::chrono::system_clock::to_time_t(now);
                std::tm tm = *std::localtime(&now_c);
                std::ostringstream ts;
                ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
                std::string timestamp = ts.str();
                std::string filename = context.outputPath + "/"
                                       + timestamp + "_"
                                       + clientContext.name + ".csv";

                /// write out CSV
                utils::PivotCSVWriter writer(filename);
                writer.write(allMetrics);

                std::cout << "Server shutting down, metrics stopped.\n";

                ioTraffic->stop();
            } else if (msg.find("END_SCENARIO") != std::string::npos) {
                std::cout << "[SERVER] End of scenario received.\n";
                // stop the server mode
            } else {
                std::cout << "[SERVER] Unknown message: " << msg << "\n";
            }
        });

        // start the read loop
        controlChannel->startReadLoop();
    });

    ioTcpController->run();

    ioTcpController->stop();
    std::cout << "[SERVER] I/O context stopped.\n";
}
