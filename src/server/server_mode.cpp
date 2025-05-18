// server_mode.cpp
#include "server_mode.h"
#include <thread>
#include <chrono>

#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <format>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <tcp_server.h>
#include <udp_server.h>
#include <yaml_parser.h>

using namespace std::chrono;

ServerMode::ServerMode(ServerContext context)
    : context_(std::move(context)), clientContext_() {
}

std::unique_ptr<INetworkServer>
ServerMode::createNetworkServer(asio::io_context &io) {
    if (context_.network.protocol == "tcp")
        return std::make_unique<TcpServer>(io, context_);
    if (context_.network.protocol == "udp")
        return std::make_unique<UdpServer>(io, context_);
    throw std::runtime_error("Unsupported protocol: " + context_.network.protocol);
}

std::vector<Metric> ServerMode::combineMetrics() {
    std::vector<Metric> all;
    all.reserve(networkMetrics_.size() + cpuMetrics_.size());

    // move‐append network then CPU
    std::ranges::move(networkMetrics_, std::back_inserter(all));
    networkMetrics_.clear();

    std::ranges::move(cpuMetrics_, std::back_inserter(all));
    cpuMetrics_.clear();

    return all;
}

void ServerMode::writeToCsv(const std::vector<Metric> &metrics) const {
    // ensure output dir exists
    std::filesystem::create_directories(context_.outputPath);

    auto now = system_clock::now();
    const auto ts = std::format("{:%Y%m%d_%H%M%S}", now);
    const auto filename = context_.outputPath + "/"
                          + ts + "_"
                          + context_.network.protocol + "_"
                          + clientContext_.name
                          + ".csv";

    const utils::PivotCSVWriter writer(filename);
    writer.write(metrics);
    LOG_INFO("Metrics written to " << filename);
}

void ServerMode::startAllMonitors() {
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

void ServerMode::stopAllMonitors() const {
    if (unifiedMonitorController_) unifiedMonitorController_->stop();
    if (networkMonitor_) networkMonitor_->stop();
    if (cpuMonitor_) cpuMonitor_->stop();
    LOG_INFO("Monitors stopped");
}

void ServerMode::run() {
    while (true) {
        isReset_ = false;
        ioContext_ = std::make_shared<asio::io_context>();

        // — create the acceptor —
        auto acceptor = std::make_shared<asio::ip::tcp::acceptor>(
            *ioContext_,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONTROL_PORT)
        );
        LOG_INFO("Accepting connections on port " << CONTROL_PORT);

        // — recursive async_accept loop —
        auto doAccept = std::make_shared<std::function<void()> >();
        *doAccept = [this, acceptor, doAccept]() {
            acceptor->async_accept(
                [this, acceptor, doAccept](const std::error_code &ec,
                                           asio::ip::tcp::socket sock) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted
                            && ec.value() != EBADF) {
                            LOG_ERROR("Accept error: " << ec.message());
                        }
                        return; // stop re-arming on error
                    }

                    // reject if already connected
                    if (controlChannel_ && controlChannel_->isOpen()) {
                        LOG_WARN("Rejecting new client; already connected");
                        std::error_code ignored;
                        sock.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
                        sock.close(ignored);
                        (*doAccept)();
                        return;
                    }

                    // happy path
                    LOG_INFO("Client connected");
                    controlChannel_ = std::make_shared<TcpControlChannel>(*ioContext_);
                    controlChannel_->assignSocket(std::move(sock));

                    controlChannel_->setDisconnectCallback([this, acceptor]() {
                        LOG_WARN("Control channel disconnected unexpectedly");
                        isReset_ = true;
                        acceptor->close();
                        ioContext_->stop();
                    });

                    controlChannel_->setMessageCallback(
                        [this, acceptor, doAccept](const std::string &msg) {
                            if (msg.rfind("scenario:", 0) == 0) {
                                clientContext_ = YamlParser::parseClientContextFromString(msg);
                                startAllMonitors();
                                baseServer_ = createNetworkServer(*ioContext_);
                                baseServer_->start();
                                controlChannel_->send("START_SCENARIO\n");
                            } else if (msg == "END_SCENARIO") {
                                // 1) stop generating traffic immediately
                                if (baseServer_) {
                                    baseServer_->stop();
                                }

                                // 2) wait one more interval so our periodic callbacks fire
                                std::this_thread::sleep_for(
                                    std::chrono::duration<double>(context_.measurement.intervalSec)
                                );


                                // 3) now stop all monitors
                                stopAllMonitors();

                                // 4) collect + output metrics
                                const auto allMetrics = combineMetrics();
                                utils::PrettyPrinter::printPivotTable(allMetrics);
                                writeToCsv(allMetrics);

                                // 5) reset collectors for next run
                                cpuCollectors_.clear();
                                networkCollectors_.clear();

                                isReset_ = true;
                                ioContext_->stop();
                            } else {
                                LOG_WARN("Unknown message: " << msg);
                            }
                        }
                    );

                    controlChannel_->startReadLoop();
                    LOG_INFO("Control channel created");
                    (*doAccept)();
                }
            );
        };

        (*doAccept)();

        // — run io_context on thread pool —
        auto guard = asio::make_work_guard(*ioContext_);
        unsigned n = std::thread::hardware_concurrency();
        if (n == 0) n = 1;
        std::vector<std::thread> pool;
        for (unsigned i = 1; i < n; ++i) {
            pool.emplace_back([ctx = ioContext_] { ctx->run(); });
        }

        ioContext_->run();
        LOG_INFO("I/O context stopped");

        guard.reset();
        for (auto &t: pool) if (t.joinable()) t.join();

        // — cleanup on reset —
        if (isReset_) {
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
