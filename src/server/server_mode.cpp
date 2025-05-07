// src/server/server_mode.cpp
#include "server_mode.h"
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "cpu_usage_monitor.h"
#include "metric_collector_factory.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "udp_server.h"
#include "tcp_server.h"
#include "utils/csv_writer.h"
#include "utils/pretty_printer.h"

ServerMode::ServerMode(ServerContext context)
    : context(std::move(context)) {
}

void ServerMode::run() const {
    while (true) {
        std::cout << "Running server mode: " << context.name << "\n";

        asio::io_context io;

        /// two different metric stores because of race conditions, to save time on lock mechanism. merge at the end.
        std::vector<Metric> cpuMetrics;
        std::vector<Metric> networkMetrics;

        // 1) pick TCP or UDP
        std::unique_ptr<INetworkServer> baseServer;
        if (context.network.protocol == "tcp") {
            baseServer = std::make_unique<TcpServer>(io, context);
        } else if (context.network.protocol == "udp") {
            baseServer = std::make_unique<UdpServer>(io, context);
        } else {
            std::cerr << "Unsupported protocol: " << context.network.protocol << "\n";
            return;
        }
        // ----------------------------------------------------

        // 2) start the server
        baseServer->start();
        std::cout << "[INFO] Server started on port: " << context.network.port << "\n";
        // ----------------------------------------------------

        /// append the filename
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_c);
        std::ostringstream ts;
        ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
        std::string timestamp = ts.str();
        std::string filename = context.outputPath + "/"
                               + timestamp + "_"
                               + context.name + ".csv";
        // ----------------------------------------------------

        // 3) set the network monitor
        PortTrafficMonitor networkMonitor(
            context.measurement.interface,
            context.network.port,
            context.measurement.intervalSec
        );
        CPUUsageMonitor cpuMonitor(
            context.measurement.intervalSec
        );
        // ----------------------------------------------------
        // 2) network callback
        auto network_collectors = MetricCollectorFactory::create(context.measurement, networkMetrics);
        networkMonitor.start(
          [&](auto in, auto out){
            for (auto& c : network_collectors)
                c->onNetworkSample(std::chrono::system_clock::now(), in, out);
            }
          );

        // 3) CPU callback
        auto cpu_collectors = MetricCollectorFactory::create(context.measurement, cpuMetrics);
        cpuMonitor.start(
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

        // ----------------------------------------------------

        // 5) run until client disconnects (io.stop is called internally)
        io.run();

        // 6) then stop the monitors
        networkMonitor.stop();
        cpuMonitor.stop();

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

        utils::PrettyPrinter::printPivotTable(allMetrics);

        /// create directories if they do not exist
        std::filesystem::create_directories(context.outputPath);

        /// write out CSV
        utils::PivotCSVWriter writer(filename);
        writer.write(allMetrics);

        std::cout << "Server shutting down, metrics stopped.\n";

        // 7) prepare for next client
        io.restart();
    }
}
