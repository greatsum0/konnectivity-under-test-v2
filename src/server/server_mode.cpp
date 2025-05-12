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


std::vector<Metric> ServerMode::combineMetrics() {
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

void ServerMode::writeToCsv(std::vector<Metric> metrics) const {
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
                           + context.network.protocol + "_"
                           + clientContext.name + ".csv";

    /// write out CSV
    utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
}

void ServerMode::startNetworkMonitor(PortTrafficMonitor &monitor,
                                     const std::vector<std::unique_ptr<IMetricCollector> > &network_collectors) {
    monitor.start(
        [&](const auto in, const auto out) {
            for (const auto &c: network_collectors)
                c->onNetworkSample(std::chrono::system_clock::now(), in, out);
        }
    );
}


void ServerMode::startCpuMonitor(CPUUsageMonitor &monitor,
                                 const std::vector<std::unique_ptr<IMetricCollector> > &cpu_collectors) {
    monitor.start(
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
}

void ServerMode::run() {
    while (true) {
        // 1) Single io_context for everything
        ioContext = std::make_shared<asio::io_context>();

        // 2) TCP acceptor on CONTROL_PORT
        auto acceptor = std::make_shared<asio::ip::tcp::acceptor>(
            *ioContext,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONTROL_PORT)
        );

        acceptor->async_accept(
            [this, acceptor](std::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
                    return;
                }

                std::cout << "[SERVER] Client connected.\n";

                // 3) Set up control channel on the same io_context
                controlChannel = std::make_shared<TcpControlChannel>(*ioContext);
                controlChannel->assignSocket(std::move(socket));
                controlChannel->setMessageCallback(
                    [this, acceptor](const std::string &msg) {
                        std::cout << "[SERVER] Message received: " << msg << "\n";

                        if (msg.find("scenario:") != std::string::npos) {
                            std::cout << "[SERVER] Scenario received, sending START_SCENARIO\n";
                            clientContext = YamlParser::parseClientContextFromString(msg);

                            // --- start monitors as before ---
                            networkMonitor_ = std::make_unique<PortTrafficMonitor>(
                                context.measurement.interface,
                                context.network.port,
                                context.measurement.intervalSec
                            );
                            networkCollectors_ = MetricCollectorFactory::create(
                                context.measurement, networkMetrics
                            );
                            startNetworkMonitor(*networkMonitor_, networkCollectors_);

                            cpuMonitor_ = std::make_unique<CPUUsageMonitor>(
                                context.measurement.intervalSec
                            );
                            cpuCollectors_ = MetricCollectorFactory::create(
                                context.measurement, cpuMetrics
                            );
                            startCpuMonitor(*cpuMonitor_, cpuCollectors_);

                            // --- create and start your traffic server on the same io_context ---
                            baseServer = createNetworkServer(*ioContext);
                            baseServer->start();

                            controlChannel->send("START_SCENARIO\n");

                            // NOTE: no nested run() here—everything will now be handled
                            //       by the single ioContext->run() below.
                        } else if (msg.find("END_SCENARIO") != std::string::npos) {
                            std::cout << "[SERVER] End of scenario received.\n";

                            // stop monitors
                            if (networkMonitor_) {
                                networkMonitor_->stop();
                                networkMonitor_.reset();
                            }
                            if (cpuMonitor_) {
                                cpuMonitor_->stop();
                                cpuMonitor_.reset();
                            }

                            // stop traffic server
                            if (baseServer) {
                                baseServer->stop();
                                baseServer.reset();
                            }

                            // gather & dump metrics
                            const std::vector<Metric> allMetrics = combineMetrics();
                            utils::PrettyPrinter::printPivotTable(allMetrics);
                            writeToCsv(allMetrics);
                            std::cout << "[SERVER] Metrics written to CSV.\n";

                            cpuCollectors_.clear();
                            networkCollectors_.clear();

                            // close only control socket
                            if (controlChannel) {
                                controlChannel->close();
                            }

                            // cancel & close the acceptor so no new clients
                            if (acceptor) {
                                acceptor->cancel();
                                acceptor->close();
                            }

                            // at this point, all sockets/acceptors are closed,
                            // so ioContext.run() will exit naturally.
                        } else {
                            std::cout << "[SERVER] Unknown message: " << msg << "\n";
                        }
                    }
                );

                std::cout << "[SERVER] Control channel created.\n";
                controlChannel->startReadLoop();
            }
        );

        std::cout << "[SERVER] Accepting connections on port " << CONTROL_PORT << "\n";

        // 4) Single event loop for all async work
        ioContext->run();

        std::cout << "[SERVER] I/O context stopped.\n";

        // Cleanup
        controlChannel.reset();
        ioContext.reset();
    }
}
