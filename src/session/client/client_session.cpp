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
#include <thread>
#include <vector>
#include <boost/asio/executor_work_guard.hpp>

#include "metrics_io.h"
#ifdef __linux__
#  include <pthread.h>
#endif
#define CONTROL_PORT 6070

ClientSession::ClientSession(ClientContext &context)
    : context_(context),
      ioContext_(std::make_shared<boost::asio::io_context>()),
      cpuIoContext_(std::make_shared<boost::asio::io_context>()),
      networkIoContext_(std::make_shared<boost::asio::io_context>()),
      cpuWorkGuard_(boost::asio::make_work_guard(*cpuIoContext_)),
      networkWorkGuard_(boost::asio::make_work_guard(*networkIoContext_)) {
}

std::shared_ptr<INetworkClient> ClientSession::createNetworkClient(
    boost::asio::io_context &ioContext,
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
    std::promise<void> readyPromise, scenarioDonePromise;
    const auto readyFuture = readyPromise.get_future();
    const auto scenarioDoneFuture = scenarioDonePromise.get_future();

    // Create a controller
    controller_ = std::make_shared<Controller>(*ioContext_,
                                               [this, &readyPromise, &scenarioDonePromise](const std::string &msg) {
                                                   if (msg == "server_ready") {
                                                       Logger::instance().info(
                                                           "Received server_ready, starting traffic setup");
                                                       readyPromise.set_value(); // wake up run()
                                                   } else if (msg == "scenario_finished_ack") {
                                                       Logger::instance().info("Received scenario_finished_ack");
                                                       scenarioDonePromise.set_value();
                                                   } else {
                                                       Logger::instance().debug("msg: " + msg);
                                                   }
                                               });

    try {
        Logger::instance().info(
            "Connecting to TCP communication server at "
            + context_.network.serverAddress + ":"
            + std::to_string(CONTROL_PORT));
        controller_->startClient(context_.network.serverAddress, CONTROL_PORT);

        auto workGuard = boost::asio::make_work_guard(*ioContext_);
        // get core count
        unsigned int numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 1;

        std::vector<std::thread> ioThreads;

        if (numCores == 1) {
            // single‐core: pin everything to core 0
            Logger::instance().info(
                "Single core detected; all threads will run on core 0");

            Logger::instance().info("Launching 1 IO thread");
            ioThreads.emplace_back([this]() {
                this->pinThread2Core(0);
                ioContext_->run();
            });

            Logger::instance().info(
                "Launching 1 CPU monitor thread and 1 network monitor thread");
            cpuThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                cpuIoContext_->run();
            });
            networkThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                networkIoContext_->run();
            });
        } else {
            // we want exactly one core for CPU (0), one for network (1),
            // and the rest for control I/O
            unsigned int cpuCores = 1;
            unsigned int networkCores = 1;
            unsigned int controlCores = numCores > 2 ? numCores - 2 : 1;
            Logger::instance().info("Launching "
                                    + std::to_string(controlCores) + " IO threads");

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
            Logger::instance().info(
                "Launching 1 CPU monitor thread and 1 network monitor thread");
            cpuThreads_.emplace_back([this]() {
                this->pinThread2Core(0);
                cpuIoContext_->run();
            });
            networkThreads_.emplace_back([this]() {
                this->pinThread2Core(1);
                networkIoContext_->run();
            });
        }

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

        baseClient_->stop();
        Logger::instance().info("Base client stopped.");
        controller_->stop();
        Logger::instance().info("Controller stopped.");

        const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(context_.measurement.intervalSec));
        Logger::instance().info("Waiting extra interval for final metric collection...");
        std::this_thread::sleep_for(interval);

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
        utils::writeMetricsToCsv(allMetrics,context_.outputPath,
                                 context_.name, context_.network.protocol);
        // 5) reset collectors for next run
        cpuCollectors_.clear();
        networkCollectors_.clear();
    } catch (const std::exception &e) {
        Logger::instance().error("Exception: " + std::string(e.what()));
        throw std::runtime_error("Failed to start client session: "
                                 + std::string(e.what()));
    }

    return 0;
}

void ClientSession::pinThread2Core(unsigned coreId) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#endif
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
               const AggregateSample &netSnap) {
            for (const auto &c: networkCollectors_) {
                c->onNetworkSample(
                    ts,
                    netSnap.bytesIn,
                    netSnap.bytesOut,
                    netSnap.packetsIn,
                    netSnap.packetsOut,
                    netSnap.throughputMbps
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

void ClientSession::stopAllMonitors() {
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
