#include "client_session.h"
#include "logger.h"
#include <controller.h>
#include <csv_writer.h>
#include <metric_collector_factory.h>
#include <pretty_printer.h>
#include <tcp_client.h>
#include <udp_client.h>
#include <traffic_pattern_factory.h>
#include <yaml_parser.h>
#include <filesystem>
#define CONTROL_PORT 6070

ClientSession::ClientSession(ClientContext &context): context_(context),
                                                      ioContext_(std::make_shared<boost::asio::io_context>()) {
}

std::shared_ptr<INetworkClient> ClientSession::createNetworkClient(boost::asio::io_context &ioContext,
                                                                   const std::string &host,
                                                                   unsigned short port) const {
    if (context_.network.protocol == "tcp")
        return std::make_shared<TcpClient>(ioContext, host, port, [this](const std::string) {
        });
    if (context_.network.protocol == "udp")
        return std::make_shared<UdpClient>(ioContext, host, port, [this](const std::string &) {
        });
    throw std::runtime_error("Unsupported protocol: " + context_.network.protocol);
}

int ClientSession::run() {
    std::promise<void> readyPromise;
    const auto readyFuture = readyPromise.get_future();

    std::promise<void> scenarioDonePromise;
    const auto scenarioDoneFuture = scenarioDonePromise.get_future();

    // Create a controller
    controller_ = std::make_shared<Controller>(*ioContext_,
                                               [this, &readyPromise, &scenarioDonePromise](const std::string &msg) {
                                                   if (msg == "server_ready") {
                                                       Logger::instance().info("Server is ready, unlocking run()");
                                                       readyPromise.set_value(); // wake up run()
                                                   } else if (msg == "scenario_finished_ack") {
                                                       Logger::instance().info("Acknowledge received, sending ack_ack and unlocking");
                                                       scenarioDonePromise.set_value();
                                                   } else {
                                                       Logger::instance().debug("msg: " + msg);
                                                   }
                                               });

    try {
        if (context_.network.protocol == "tcp") {
            Logger::instance().info(
                "Connecting to TCP server at " + context_.network.serverAddress + ":" + std::to_string(
                    CONTROL_PORT));
            controller_->startClient(context_.network.serverAddress, CONTROL_PORT);
        } else if (context_.network.protocol == "udp") {
            Logger::instance().info(
                "Connecting to UDP server at " + context_.network.serverAddress + ":" + std::to_string(
                    CONTROL_PORT));
            controller_->startClient(context_.network.serverAddress, CONTROL_PORT);
        } else {
            Logger::instance().error("Unknown protocol: " + context_.network.protocol);
            throw std::runtime_error("Unknown protocol: " + context_.network.protocol);
        }

        // Run the I/O context
        std::thread io_thread([&]() {
            ioContext_->run();
        });

        const auto yaml = YamlParser::toYamlString(context_);
        controller_->send(yaml);

        readyFuture.wait();
        Logger::instance().info("Continuing with traffic setup...");

        startAllMonitors();
        Logger::instance().info("Starting scenario now!");
        baseClient_ = createNetworkClient(*ioContext_,
                                          context_.network.serverAddress,
                                          context_.network.serverPort);

        baseClient_->start();
        Logger::instance().info("Network client started.");
        pattern_ = TrafficPatternFactory::create(context_.traffic);
        Logger::instance().info("Traffic pattern created.");

        trafficScheduler_ = std::make_shared<TrafficScheduler>(
            *ioContext_,
            *baseClient_,
            context_,
            *pattern_,
            [this]() {
                Logger::instance().info("Notify server of traffic completion");
                controller_->send("scenario_finished");
            });

        trafficScheduler_->start();
        Logger::instance().info("Traffic scheduler started.");

        scenarioDoneFuture.wait();

        controller_->stop();
        baseClient_->stop();
        ioContext_->stop();

        Logger::instance().info("Client stopped.");

        if (io_thread.joinable())
            io_thread.join();
        stopAllMonitors();

        const auto allMetrics = combineMetrics();
        utils::PrettyPrinter::printPivotTable(allMetrics);
        writeToCsv(allMetrics);

        // 5) reset collectors for next run
        cpuCollectors_.clear();
        networkCollectors_.clear();
    } catch (const std::exception &e) {
        Logger::instance().error("Exception: " + std::string(e.what()));
        throw std::runtime_error("Failed to start client session: " + std::string(e.what()));
    }

    return 0;
}


void ClientSession::startAllMonitors() {
    // network
    networkMonitor_ = std::make_shared<PortTrafficMonitor>(
        context_.measurement.interface,
        context_.network.serverPort,
        context_.measurement.intervalSec
    );
    networkMonitor_->start(nullptr);
    networkCollectors_ = MetricCollectorFactory::create(
        context_.measurement, networkMetrics_);

    // cpu
    cpuMonitor_ = std::make_shared<CPUUsageMonitor>(
        context_.measurement.intervalSec
    );

    cpuCollectors_ = MetricCollectorFactory::create(
        context_.measurement, cpuMetrics_);

    // unified controller
    unifiedMonitorController_ = std::make_unique<UnifiedMonitorController>(
        *ioContext_, cpuMonitor_, networkMonitor_, context_.measurement.intervalSec
    );

    unifiedMonitorController_->start(
        [this](std::chrono::system_clock::time_point ts,
               const CPUUsageSnapshot &cpuSnap,
               const PortTrafficMonitor::AggregateSample &netSnap) {
            for (const auto &c: networkCollectors_) {
                c->onNetworkSample(
                    ts,
                    netSnap.bytesIn, netSnap.bytesOut,
                    netSnap.packetsIn, netSnap.packetsOut
                );
            }
            for (auto &c: cpuCollectors_) {
                c->onCpuSample(ts, cpuSnap.totalSum, cpuSnap.selfCpu);
                c->onCpuDetailSample(
                    ts,
                    cpuSnap.perCoreUsages,
                    cpuSnap.selfShareVsTotal,
                    cpuSnap.selfShareVsSystemCap
                );
            }
        }
    );
    Logger::instance().info("Monitors started");
}

void ClientSession::stopAllMonitors() const {
    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();
    Logger::instance().info("Monitors stopped");
}

std::vector<Metric> ClientSession::combineMetrics() {
    std::vector<Metric> all;
    all.reserve(networkMetrics_.size() + cpuMetrics_.size());

    // move‐append network then CPU
    std::ranges::move(networkMetrics_, std::back_inserter(all));
    networkMetrics_.clear();

    std::ranges::move(cpuMetrics_, std::back_inserter(all));
    cpuMetrics_.clear();

    return all;
}

void ClientSession::writeToCsv(const std::vector<Metric> &metrics) const {
    // ensure output dir exists
    std::filesystem::create_directories(context_.outputPath);

    auto now = std::chrono::system_clock::now();
    const auto ts = std::format("{:%Y%m%d_%H%M%S}", now);
    const auto filename = context_.outputPath + "/"
                          + ts + "_"
                          + context_.network.protocol + "_"
                          + context_.name
                          + ".csv";

    const utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
    Logger::instance().info("Metrics written to " + filename);
}
