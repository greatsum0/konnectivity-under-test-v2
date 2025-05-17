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

ClientMode::ClientMode(ClientContext context)
    : context_(std::move(context)) {
}

std::vector<Metric> ClientMode::combineMetrics() {
    std::vector<Metric> allMetrics;
    allMetrics.reserve(networkMetrics_.size() + cpuMetrics_.size());

    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(networkMetrics_.begin()),
                      std::make_move_iterator(networkMetrics_.end()));
    networkMetrics_.clear();

    allMetrics.insert(allMetrics.end(),
                      std::make_move_iterator(cpuMetrics_.begin()),
                      std::make_move_iterator(cpuMetrics_.end()));
    cpuMetrics_.clear();

    return allMetrics;
}

void ClientMode::writeToCsv(std::vector<Metric> metrics) const {
    std::filesystem::create_directories(context_.outputPath);

    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");

    std::string filename = context_.outputPath + "/" +
                           ts.str() + "_" +
                           context_.network.protocol + "_" +
                           context_.name + ".csv";

    utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
}

void ClientMode::startAllMonitors(const std::chrono::system_clock::time_point &referenceTime) {
    networkMonitor_ = std::make_shared<PortTrafficMonitor>(
        context_.measurement.interface,
        context_.network.serverPort,
        context_.measurement.intervalSec
    );
    networkMonitor_->start(nullptr);
    networkCollectors_ = MetricCollectorFactory::create(
        context_.measurement, networkMetrics_
    );
    cpuMonitor_ = std::make_shared<CPUUsageMonitor>(
        context_.measurement.intervalSec
    );

    cpuCollectors_ = MetricCollectorFactory::create(
        context_.measurement, cpuMetrics_
    );
    unifiedMonitorController_ = std::make_unique<UnifiedMonitorController>(
        *ioContext_, cpuMonitor_, networkMonitor_, context_.measurement.intervalSec
    );

    unifiedMonitorController_->start(
        [this](std::chrono::system_clock::time_point ts,
               const CPUUsageSnapshot &cpuSnap,
               const PortTrafficMonitor::AggregateSample &netSnap) {
            for (const auto &c: networkCollectors_) {
                c->onNetworkSample(ts, netSnap.bytesIn, netSnap.bytesOut, netSnap.packetsIn, netSnap.packetsOut);
            }

            for (const auto &c: cpuCollectors_) {
                c->onCpuSample(ts, cpuSnap.totalSum, cpuSnap.selfCpu);
                c->onCpuDetailSample(ts, cpuSnap.perCoreUsages, cpuSnap.selfShareVsTotal, cpuSnap.selfShareVsSystemCap);
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

void ClientMode::startTraffic() {
    baseClient_ = createNetworkClient(*ioContext_);
    baseClient_->connect();
    std::cout << "[INFO] Connected to " << context_.network.serverAddress << ":" << context_.network.serverPort << "\n";

    startAllMonitors(std::chrono::system_clock::now());

    pattern_ = TrafficPatternFactory::create(context_.traffic);

    TrafficScheduler trafficSched(*ioContext_, *baseClient_, context_, *pattern_);
    trafficSched.start();

    std::cout << "[INFO] Traffic scheduler started\n";

    const auto exitTimer = std::make_shared<asio::steady_timer>(*ioContext_);
    exitTimer->expires_after(
        std::chrono::duration_cast<asio::steady_timer::duration>(
            std::chrono::duration<double>(context_.traffic.durationSec)
        )
    );

    exitTimer->async_wait([this](const asio::error_code &) {
        std::cout << "[INFO] Duration elapsed, posting shutdown steps\n";
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

    std::cout << "[INFO] Running I/O context...\n";
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
            std::cout << "[APP] Server accepted scenario. Starting traffic...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            startTraffic();
            controlChannel_->close(); // cleanly close control channel
        }
    });

    controlChannel_->startReadLoop();
    controlChannel_->sendScenarioFile(context_);
    std::cout << "[INFO] Scenario file sent to server\n";

    // Main thread waits for the control channel to finish
    for (auto &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();

    const auto allMetrics = combineMetrics();
    utils::PrettyPrinter::printPivotTable(allMetrics);
    writeToCsv(allMetrics);
    std::cout << "[CLIENT] Metrics written to CSV.\n";

    cpuCollectors_.clear();
    networkCollectors_.clear();
    controlChannel_.reset();
    baseClient_.reset();

    workGuard.reset(); // clean up the work guard
    ioContext_.reset();
}
