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
    : context_(std::move(context)) {
}

std::vector<Metric> ClientMode::combineMetrics() {
    std::vector<Metric> allMetrics;

    /// append all metrics to a single vector
    /// append network metrics first
    allMetrics.reserve(networkMetrics_.size() + cpuMetrics_.size());
    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(networkMetrics_.begin()),
                      std::make_move_iterator(networkMetrics_.end()));

    /// clear network metrics to avoid bloating the memory
    networkMetrics_.clear();

    /// append cpu metrics
    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(cpuMetrics_.begin()),
                      std::make_move_iterator(cpuMetrics_.end()));
    /// clear cpu metrics to avoid bloating the memory
    cpuMetrics_.clear();

    return allMetrics;
}

void ClientMode::writeToCsv(std::vector<Metric> metrics) const {
    /// create directories if they do not exist
    std::filesystem::create_directories(context_.outputPath);

    /// append the filename
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string timestamp = ts.str();
    std::string filename = context_.outputPath + "/"
                           + timestamp + "_"
                           + context_.name + ".csv";

    /// write out CSV
    utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
}

void ClientMode::startNetworkMonitor(PortTrafficMonitor &monitor,
                                     const std::vector<std::unique_ptr<IMetricCollector> > &network_collectors) {
    monitor.start(
        [&](const auto in, const auto out) {
            for (const auto &c: network_collectors)
                c->onNetworkSample(std::chrono::system_clock::now(), in, out);
        }
    );
}


void ClientMode::startCpuMonitor(CPUUsageMonitor &monitor,
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


std::unique_ptr<INetworkClient> ClientMode::createNetworkClient(asio::io_context &io) const {
    if (context_.network.protocol == "tcp") {
        return std::make_unique<TcpClient>(io, context_);
    }
    if (context_.network.protocol == "udp") {
        return std::make_unique<UdpClient>(io, context_);
    }
    throw std::runtime_error("Unsupported protocol: " + context_.network.protocol);
}

void ClientMode::startTraffic(const std::shared_ptr<TcpControlChannel> &controlChannel) {
    baseClient_ = createNetworkClient(*ioContext_);
    baseClient_->connect();
    std::cout << "[INFO] Connected to " << context_.network.serverAddress << ":" << context_.network.serverPort << "\n";

    networkMonitor_ = std::make_unique<PortTrafficMonitor>(
        context_.measurement.interface,
        context_.network.serverPort,
        context_.measurement.intervalSec
    );
    networkCollectors_ = MetricCollectorFactory::create(
        context_.measurement, networkMetrics_
    );
    startNetworkMonitor(*networkMonitor_, networkCollectors_);

    cpuMonitor_ = std::make_unique<CPUUsageMonitor>(
        context_.measurement.intervalSec
    );
    cpuCollectors_ = MetricCollectorFactory::create(
        context_.measurement, cpuMetrics_
    );
    startCpuMonitor(*cpuMonitor_, cpuCollectors_);

    pattern_ = TrafficPatternFactory::create(context_.traffic);

    TrafficScheduler trafficSched(
        *ioContext_,
        *baseClient_,
        context_,
        *pattern_
    );
    trafficSched.start();
    std::cout << "[INFO] Traffic scheduler started\n";

    const auto exitTimer = std::make_shared<asio::steady_timer>(*ioContext_);
    exitTimer->expires_after(
        std::chrono::duration_cast<asio::steady_timer::duration>(
            std::chrono::duration<double>(context_.traffic.durationSec)
        )
    );

    exitTimer->async_wait([this](const asio::error_code &) {
        std::cout << "[INFO] Duration elapsed, stopping I/O context\n";
        ioContext_->stop();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "[INFO] Stopping traffic scheduler and disconnecting...\n";

        controlChannel_->send("END_SCENARIO");
    });

    std::cout << "[INFO] Running I/O context...\n";
    ioContext_->run();
}


void ClientMode::run() {
    // 1) Single io_context for everything
    ioContext_ = std::make_shared<asio::io_context>();

    controlChannel_ = std::make_shared<TcpControlChannel>(*ioContext_);
    controlChannel_->connect(context_.network.serverAddress, CONTROL_PORT);

    controlChannel_->setMessageCallback([this](const std::string &msg) {
        if (msg.find("START_SCENARIO") != std::string::npos) {
            std::cout << "[APP] Server accepted scenario. Starting traffic...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            startTraffic(controlChannel_);
            controlChannel_->close();
        }
    });

    controlChannel_->startReadLoop();
    controlChannel_->sendScenarioFile(context_);
    std::cout << "[INFO] Scenario file sent to server\n";

    ioContext_->run();

    // stop monitors
    if (networkMonitor_) {
        networkMonitor_->stop();
        networkMonitor_.reset();
    }
    if (cpuMonitor_) {
        cpuMonitor_->stop();
        cpuMonitor_.reset();
    }

    const std::vector<Metric> allMetrics = combineMetrics();
    utils::PrettyPrinter::printPivotTable(allMetrics);
    writeToCsv(allMetrics);
    std::cout << "[CLIENT] Metrics written to CSV.\n";

    cpuCollectors_.clear();
    networkCollectors_.clear();

    controlChannel_.reset();
    ioContext_.reset();
}
