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


ServerSession::ServerSession(ServerContext &context)
    : context_(context), clientContext_(),
      ioContext_(std::make_shared<boost::asio::io_context>()),
      cpuIoContext_(std::make_shared<boost::asio::io_context>()),
      networkIoContext_(std::make_shared<boost::asio::io_context>()),
      cpuWorkGuard_(boost::asio::make_work_guard(*cpuIoContext_)),
      networkWorkGuard_(boost::asio::make_work_guard(*networkIoContext_)) {
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
    std::promise<void> scenarioDonePromise, scenarioReadyPromise, scenarioFinishedAckPromise;
    const auto scenarioDoneFuture = scenarioDonePromise.get_future();
    const auto scenarioReadyFuture = scenarioReadyPromise.get_future();
    const auto scenarioFinishedAckFuture = scenarioFinishedAckPromise.get_future();
    // Create a controller
    controller_ = std::make_shared<Controller>(*ioContext_,
                                               [this, &scenarioDonePromise, &scenarioReadyPromise, &
                                                   scenarioFinishedAckPromise](
                                           const std::string &msg) {
                                                   if (msg.rfind("scenario:", 0) == 0) {
                                                       clientContext_ = YamlParser::parseClientContextFromString(msg);
                                                       Logger::instance().info(
                                                           "Received scenario: " + clientContext_.name);
                                                       scenarioReadyPromise.set_value();
                                                   } else if (msg.rfind("scenario_finished", 0) == 0) {
                                                       Logger::instance().info(
                                                           "Received scenario_finished");
                                                       scenarioFinishedAckPromise.set_value();

                                                       Logger::instance().info("Sent scenario_finished_ack");
                                                       scenarioDonePromise.set_value();
                                                   } else {
                                                       Logger::instance().debug("msg: " + msg);
                                                   }
                                               });

    try {
        Logger::instance().info("Starting TCP controller server on port " + std::to_string(CONTROL_PORT));
        controller_->startServer(CONTROL_PORT);

        auto workGuard = boost::asio::make_work_guard(*ioContext_);
        // get core count
        unsigned int numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 1;

        std::vector<std::thread> ioThreads;

        if (numCores == 1) {
            // single‐core: pin everything to core 0
            Logger::instance().info("Single core detected; all threads will run on core 0");

            Logger::instance().info("Launching 1 IO thread");
            ioThreads.emplace_back([this]() {
                this->pinThread2Core(0);
                ioContext_->run();
            });

            Logger::instance().info("Launching 1 CPU monitor thread and 1 network monitor thread");
            cpuThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                cpuIoContext_->run();
            });
            networkThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                networkIoContext_->run();
            });
        } else {
            // we want exactly one core for CPU monitor (core 0),
            // one for network monitor (core 1), and the rest for control I/O
            unsigned int cpuCores = 1;
            unsigned int networkCores = 1;
            unsigned int controlCores = numCores > 2 ? numCores - 2 : 1;
            Logger::instance().info("Launching " + std::to_string(controlCores) + " IO threads");

            ioThreads.reserve(controlCores);
            // spawn & pin control I/O threads
            for (unsigned core = 0; core < controlCores; ++core) {
                unsigned actualCore = cpuCores + networkCores + core;
                ioThreads.emplace_back([this, actualCore]() {
                    this->pinThread2Core(actualCore);
                    ioContext_->run();
                });
            }

            // spawn exactly 1 CPU monitor thread and 1 network monitor thread
            Logger::instance().info("Launching 1 CPU monitor thread and 1 network monitor thread");
            // CPU thread on core 0
            cpuThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                cpuIoContext_->run();
            });
            // Network thread on core 1
            networkThreads_.emplace_back([this]() {
                this->pinThread2Core(1);
                networkIoContext_->run();
            });
        }

        scenarioReadyFuture.wait();

        startAllMonitors();
        baseServer_ = createNetworkServer(*ioContext_, context_.network.port);
        baseServer_->start();
        Logger::instance().info("Network server started on port " + std::to_string(context_.network.port));
        controller_->send("server_ready");
        Logger::instance().info("Sent server_ready");

        scenarioFinishedAckFuture.wait();
        controller_->send("scenario_finished_ack");
        baseServer_->stop();
        // Wait for the scenario to finish
        scenarioDoneFuture.wait();
        Logger::instance().info("Handshake complete; stopping server.");

        controller_->stop();

        // let run() return by dropping the work guard
        workGuard.reset();
        Logger::instance().info("Stopping IO context and joining threads...");
        ioContext_->stop();

        for (auto &t: ioThreads) {
            if (t.joinable()) t.join();
        }

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

void ServerSession::pinThread2Core(unsigned coreId) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#endif
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

void ServerSession::stopAllMonitors() {
    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();

    // teardown CPU pool
    cpuWorkGuard_.reset();
    if (cpuIoContext_) cpuIoContext_->stop();
    for (auto &t: cpuThreads_) {
        if (t.joinable()) t.join();
    }

    // teardown Network pool
    networkWorkGuard_.reset();
    if (networkIoContext_) networkIoContext_->stop();
    for (auto &t: networkThreads_) {
        if (t.joinable()) t.join();
    }

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
