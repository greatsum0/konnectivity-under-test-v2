#include "server_session.h"

#include <csv_writer.h>
#include <metric_collector_factory.h>
#include <pretty_printer.h>

#include "logger.h"
#include <tcp_server.h>
#include <udp_server.h>
#include <yaml_parser.h>
#include <filesystem>
#define CONTROL_PORT 6070

ServerSession::ServerSession(ServerContext &context): context_(context), clientContext_(),
                                                      ioContext_(std::make_shared<boost::asio::io_context>()) {
}

std::shared_ptr<INetworkServer> ServerSession::createNetworkServer(boost::asio::io_context &ioContext,
                                                                   unsigned short port) const {
    if (context_.network.protocol == "tcp")
        return std::make_shared<TcpServer>(ioContext, port, [this](const std::string &) {
        });
    if (context_.network.protocol == "udp")
        return std::make_shared<UdpServer>(ioContext, port, [this](const std::string &) {
        });
    throw std::runtime_error("Unsupported protocol: " + context_.network.protocol);
}


int ServerSession::run() {
    std::promise<void> scenarioDonePromise;
    const auto scenarioDoneFuture = scenarioDonePromise.get_future();
    // Create a controller
    controller_ = std::make_shared<Controller>(*ioContext_,
                                               [this, &scenarioDonePromise](const std::string &msg) {
                                                   if (msg.rfind("scenario:", 0) == 0) {
                                                       clientContext_ = YamlParser::parseClientContextFromString(msg);
                                                       startAllMonitors();
                                                       Logger::instance().info(
                                                           "Received scenario: " + clientContext_.name);
                                                       baseServer_ = createNetworkServer(
                                                           *ioContext_, context_.network.port);
                                                       Logger::instance().info("Network server created.");
                                                       baseServer_->start();
                                                       Logger::instance().info("Network server started.");
                                                       controller_->send("server_ready");
                                                       Logger::instance().info("Client notified of server readiness");
                                                   } else if (msg.rfind("scenario_finished", 0) == 0) {
                                                       Logger::instance().info("Acknowledging scenario finished");
                                                       controller_->send("scenario_finished_ack");
                                                       scenarioDonePromise.set_value();
                                                   }  else {
                                                       Logger::instance().debug("msg: " + msg);
                                                   }
                                               });

    try {
        if (context_.network.protocol == "tcp") {
            Logger::instance().info("Starting TCP server on port " + std::to_string(CONTROL_PORT));
            controller_->startServer(CONTROL_PORT);
        } else if (context_.network.protocol == "udp") {
            Logger::instance().info("Starting UDP server on port " + std::to_string(CONTROL_PORT));
            controller_->startServer(CONTROL_PORT);
        } else {
            Logger::instance().error("Unknown protocol: " + context_.network.protocol);
            throw std::runtime_error("Unknown protocol: " + context_.network.protocol);
        }

        // Run the I/O context
        std::thread io_thread([&]() {
            ioContext_->run();
        });

        // Wait for the scenario to finish
        scenarioDoneFuture.wait();
        Logger::instance().info("Scenario finished, stopping server.");
        controller_->stop();
        baseServer_->stop();
        ioContext_->stop();

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
        throw std::runtime_error("Failed to start server session: " + std::string(e.what()));
    }

    return 0;
}


void ServerSession::startAllMonitors() {
    // network
    networkMonitor_ = std::make_shared<PortTrafficMonitor>(
        context_.measurement.interface,
        context_.network.port,
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

void ServerSession::stopAllMonitors() const {
    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();
    Logger::instance().info("Monitors stopped");
}

std::vector<Metric> ServerSession::combineMetrics() {
    std::vector<Metric> all;
    all.reserve(networkMetrics_.size() + cpuMetrics_.size());

    // move‐append network then CPU
    std::ranges::move(networkMetrics_, std::back_inserter(all));
    networkMetrics_.clear();

    std::ranges::move(cpuMetrics_, std::back_inserter(all));
    cpuMetrics_.clear();

    return all;
}

void ServerSession::writeToCsv(const std::vector<Metric> &metrics) const {
    // ensure output dir exists
    std::filesystem::create_directories(context_.outputPath);

    auto now = std::chrono::system_clock::now();
    const auto ts = std::format("{:%Y%m%d_%H%M%S}", now);
    const auto filename = context_.outputPath + "/"
                          + ts + "_"
                          + context_.network.protocol + "_"
                          + clientContext_.name
                          + ".csv";

    const utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
    Logger::instance().info("Metrics written to " + filename);
}
