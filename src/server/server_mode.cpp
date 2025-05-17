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
#include <thread>
#include <vector>
#include <cerrno>
#include <asio/steady_timer.hpp>

#include "cpu_usage_monitor.h"
#include "metric_collector_factory.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "tcp_control_channel.h"
#include "udp_server.h"
#include "tcp_server.h"
#include "unified_monitor_controller.h"
#include "utils/csv_writer.h"
#include "utils/pretty_printer.h"

#define CONTROL_PORT 6070

ServerMode::ServerMode(ServerContext context)
    : context_(std::move(context)), clientContext_() {
}

std::unique_ptr<INetworkServer> ServerMode::createNetworkServer(asio::io_context &io) {
    if (context_.network.protocol == "tcp") {
        return std::make_unique<TcpServer>(io, context_);
    }
    if (context_.network.protocol == "udp") {
        return std::make_unique<UdpServer>(io, context_);
    }
    throw std::runtime_error("Unsupported protocol: " + context_.network.protocol);
}


std::vector<Metric> ServerMode::combineMetrics() {
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

void ServerMode::writeToCsv(std::vector<Metric> metrics) const {
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
                           + context_.network.protocol + "_"
                           + clientContext_.name + ".csv";

    /// write out CSV
    utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
}

void ServerMode::startAllMonitors(const std::chrono::system_clock::time_point &referenceTime) {
    networkMonitor_ = std::make_shared<PortTrafficMonitor>(
        context_.measurement.interface,
        context_.network.port,
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

void ServerMode::run() {
    while (true) {
        isReset = false;
        ioContext_ = std::make_shared<asio::io_context>();

        // — create the acceptor —
        auto acceptor = std::make_shared<asio::ip::tcp::acceptor>(
            *ioContext_,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONTROL_PORT)
        );

        // — recursive accept loop via shared_ptr to avoid dangling refs —
        auto doAccept = std::make_shared<std::function<void()> >();
        *doAccept = [this, acceptor, doAccept]() {
            acceptor->async_accept(
                [this, acceptor, doAccept](const std::error_code ec,
                                           asio::ip::tcp::socket socket) {
                    if (ec) {
                        // suppress normal shutdown errors
                        if (ec != asio::error::operation_aborted && ec.value() != EBADF) {
                            std::cerr << "[SERVER] Accept error: " << ec.message() << "\n";
                        }
                        return; // stop re-arming on error
                    }

                    // if already have a client, reject extras
                    if (controlChannel_ && controlChannel_->isOpen()) {
                        std::cerr << "[SERVER] Rejecting new client, already connected.\n";
                        std::error_code ignored;
                        socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
                        socket.close(ignored);
                        (*doAccept)();
                        return;
                    }

                    // — happy path: new control channel —
                    std::cout << "[SERVER] Client connected.\n";
                    controlChannel_ = std::make_shared<TcpControlChannel>(*ioContext_);
                    controlChannel_->assignSocket(std::move(socket));

                    controlChannel_->setDisconnectCallback([this, acceptor]() {
                        std::cerr << "[SERVER] Control channel disconnected unexpectedly.\n";
                        isReset = true;
                        acceptor->close(); // tear down acceptor
                        ioContext_->stop();
                    });

                    controlChannel_->setMessageCallback(
                        [this, acceptor, doAccept](const std::string &msg) {
                            if (msg.find("scenario:") != std::string::npos) {
                                clientContext_ = YamlParser::parseClientContextFromString(msg);
                                startAllMonitors(std::chrono::system_clock::now());
                                baseServer_ = createNetworkServer(*ioContext_);
                                baseServer_->start();
                                controlChannel_->send("START_SCENARIO\n");
                            } else if (msg.find("END_SCENARIO") != std::string::npos) {
                                // stop accepting further connections
                                acceptor->cancel();
                                acceptor->close();

                                // stop all monitors & servers
                                if (unifiedMonitorController_) unifiedMonitorController_->stop();
                                if (networkMonitor_) networkMonitor_->stop();
                                if (cpuMonitor_) cpuMonitor_->stop();
                                if (baseServer_) baseServer_->stop();

                                // collect + output metrics
                                const auto allMetrics = combineMetrics();
                                utils::PrettyPrinter::printPivotTable(allMetrics);
                                writeToCsv(allMetrics);

                                cpuCollectors_.clear();
                                networkCollectors_.clear();

                                isReset = true;
                                ioContext_->stop();
                            } else {
                                std::cout << "[SERVER] Unknown message: " << msg << "\n";
                            }
                        }
                    );

                    controlChannel_->startReadLoop();
                    std::cout << "[SERVER] Control channel created.\n";

                    // arm next accept
                    (*doAccept)();
                }
            );
        };

        // kick off first accept
        (*doAccept)();

        // ——— Multi-threaded io_context run ———

        // 1) keep context alive even if no immediate work
        auto workGuard = asio::make_work_guard(*ioContext_);

        // 2) spawn pool of (hardware) threads
        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 1;
        std::vector<std::thread> pool;
        pool.reserve(threadCount - 1);

        for (unsigned int i = 1; i < threadCount; ++i) {
            pool.emplace_back([io = ioContext_]() {
                io->run();
            });
        }

        // 3) run one in main thread
        std::cout << "[SERVER] Accepting connections on port " << CONTROL_PORT << "\n";
        ioContext_->run();
        std::cout << "[SERVER] I/O context stopped.\n";

        // 4) teardown pool
        workGuard.reset();
        for (auto &t: pool) if (t.joinable()) t.join();

        // — cleanup after reset —
        if (isReset) {
            if (controlChannel_) {
                controlChannel_->close();
                controlChannel_.reset();
            }
            if (acceptor) {
                std::error_code ec;
                acceptor->cancel(ec);
                acceptor->close(ec);
            }
            ioContext_.reset();
        }
    }
}
