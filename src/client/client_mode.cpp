#include "client_mode.h"
#include <iostream>
#include <utility>
#include <memory>
#include <vector>
#include <asio.hpp>
#include <port_traffic_monitor.h>
#include <cpu_usage_monitor.h>
#include <asio/io_context.hpp>
#include <filesystem>
#include <yaml_parser.h>

#include "metric_collector_factory.h"
#include "metric_store.h"
#include "traffic_scheduler.h"
#include "tcp_client.h"
#include "tcp_control_channel.h"
#include "udp_client.h"
#include "traffic_pattern_factory.h"
#include "utils/pretty_printer.h"
#include "utils/csv_writer.h"
#define CONTROL_PORT 6070
#include <yaml-cpp/yaml.h>


ClientMode::ClientMode(ClientContext context)
    : context(std::move(context)) {
}

std::vector<Metric> ClientMode::combineMetrics(std::vector<Metric> cpuMetrics,
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


std::unique_ptr<INetworkClient> ClientMode::createNetworkClient(asio::io_context &io) const {
    if (context.network.protocol == "tcp") {
        return std::make_unique<TcpClient>(io, context);
    }
    if (context.network.protocol == "udp") {
        return std::make_unique<UdpClient>(io, context);
    }
    throw std::runtime_error("Unsupported protocol: " + context.network.protocol);
}

void ClientMode::startTraffic(const std::shared_ptr<TcpControlChannel> &controlChannel) const {
    auto ioTraffic = std::make_shared<asio::io_context>();

    std::unique_ptr<INetworkClient> baseClient = createNetworkClient(*ioTraffic);
    baseClient->connect();
    std::cout << "[INFO] Connected to " << context.network.serverAddress << ":" << context.network.serverPort << "\n";

    const std::unique_ptr<ITrafficPattern> pattern = TrafficPatternFactory::create(context.traffic);

    TrafficScheduler trafficSched(
        *ioTraffic,
        *baseClient,
        context,
        *pattern
    );
    trafficSched.start();
    std::cout << "[INFO] Traffic scheduler started\n";

    const auto exitTimer = std::make_shared<asio::steady_timer>(*ioTraffic);
    exitTimer->expires_after(
        std::chrono::duration_cast<asio::steady_timer::duration>(
            std::chrono::duration<double>(context.traffic.durationSec)
        )
    );
    exitTimer->async_wait([ioTraffic, controlChannel, &baseClient](const asio::error_code &) {
        std::cout << "[INFO] Duration elapsed, stopping I/O context\n";
        ioTraffic->stop();
        // baseClient->disconnect();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "[INFO] Stopping traffic scheduler and disconnecting...\n";
        controlChannel->send("END_SCENARIO");
        // wait for the server to finish processing
    });

    std::cout << "[INFO] Running I/O context...\n";
    ioTraffic->run();
}


void ClientMode::run() const {
    const auto ioTcpController = std::make_shared<asio::io_context>();
    auto controlChannel = std::make_shared<TcpControlChannel>(*ioTcpController);

    controlChannel->connect(context.network.serverAddress, CONTROL_PORT);
    controlChannel->setMessageCallback([controlChannel, this](const std::string &msg) {
        if (msg.find("START_SCENARIO") != std::string::npos) {
            std::cout << "[APP] Server accepted scenario. Starting traffic...\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            startTraffic(controlChannel);
            controlChannel->close();
        }
    });

    controlChannel->startReadLoop();
    controlChannel->sendScenarioFile(context);
    std::cout << "[INFO] Scenario file sent to server\n";

    ioTcpController->run();
}
