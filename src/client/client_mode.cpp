#include "client_mode.h"
#include <iostream>
#include <utility>
#include <filesystem>
#include <asio/steady_timer.hpp>
#include <yaml_parser.h>
#include <thread>

#include "metric_collector_factory.h"
#include "metric_store.h"
#include "traffic_scheduler.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "traffic_pattern_factory.h"
#include "unified_monitor_controller.h"
#include "utils/pretty_printer.h"
#include "utils/csv_writer.h"

#define CONTROL_PORT 6070
using namespace std::chrono;

ClientMode::ClientMode(ClientContext context)
    : context_(std::move(context)) {
}

std::vector<Metric> ClientMode::combineMetrics() {
    std::vector<Metric> all;
    all.reserve(networkMetrics_.size() + cpuMetrics_.size());

    // move‐append network then CPU
    std::ranges::move(networkMetrics_, std::back_inserter(all));
    networkMetrics_.clear();

    std::ranges::move(cpuMetrics_, std::back_inserter(all));
    cpuMetrics_.clear();

    return all;
}

void ClientMode::writeToCsv(const std::vector<Metric> &metrics) const {
    // ensure output dir exists
    std::filesystem::create_directories(context_.outputPath);

    auto now = system_clock::now();
    const auto ts = std::format("{:%Y%m%d_%H%M%S}", now);
    const auto filename = context_.outputPath + "/"
                          + ts + "_"
                          + context_.network.protocol + "_"
                          + context_.name
                          + ".csv";

    const utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
    LOG_INFO("Metrics written to " << filename);
}

void ClientMode::startAllMonitors() {
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
        [this](system_clock::time_point ts,
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
    LOG_INFO("Monitors started");
}

void ClientMode::stopAllMonitors() const {
    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();
    LOG_INFO("Monitors stopped");
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

void ClientMode::startTraffic() {
    baseClient_ = createNetworkClient(*ioContext_);
    baseClient_->connect();
    LOG_INFO("Connected to " << context_.network.serverAddress << ":" << context_.network.serverPort);

    startAllMonitors();

    pattern_ = TrafficPatternFactory::create(context_.traffic);

    TrafficScheduler trafficSched(*ioContext_, *baseClient_, context_, *pattern_);
    trafficSched.start();

    LOG_INFO("Traffic scheduler started");

    const auto exitTimer = std::make_shared<asio::steady_timer>(*ioContext_);
    exitTimer->expires_after(
        std::chrono::duration_cast<asio::steady_timer::duration>(
            std::chrono::duration<double>(context_.traffic.durationSec)
        )
    );

    exitTimer->async_wait([this](const asio::error_code &) {
        LOG_INFO("Duration elapsed, posting shutdown steps");
        asio::post(*ioContext_, [this]() {
            if (controlChannel_ && controlChannel_->isOpen()) {
                controlChannel_->send("END_SCENARIO");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            if (baseClient_) {
                baseClient_->disconnect();
            }

            ioContext_->stop();
        });
    });

    LOG_INFO("Running I/O context...");
    ioContext_->run();
}

void ClientMode::run() {
    ioContext_ = std::make_shared<asio::io_context>();
    auto workGuard = asio::make_work_guard(*ioContext_);

    // start N threads to run the io_context e.g. number of cores
    const unsigned int threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    threads.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i) {
        threads.emplace_back([this]() {
            ioContext_->run();
        });
    }

    controlChannel_ = std::make_shared<TcpControlChannel>(*ioContext_);
    controlChannel_->connect(context_.network.serverAddress, CONTROL_PORT);

    controlChannel_->setMessageCallback([this](const std::string &msg) {
        if (msg.find("START_SCENARIO") != std::string::npos) {
            LOG_INFO("Server accepted scenario. Starting traffic...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            startTraffic();
            controlChannel_->close(); // cleanly close control channel
        }
    });

    controlChannel_->startReadLoop();
    controlChannel_->sendScenarioFile(context_);
    LOG_INFO("Scenario file sent to server");

    // Main thread waits for the control channel to finish
    for (auto &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    stopAllMonitors();

    const auto allMetrics = combineMetrics();
    utils::PrettyPrinter::printPivotTable(allMetrics);
    writeToCsv(allMetrics);
    LOG_INFO("Metrics written to CSV.");

    cpuCollectors_.clear();
    networkCollectors_.clear();
    controlChannel_.reset();
    baseClient_.reset();

    workGuard.reset(); // clean up the work guard
    ioContext_.reset();
}
