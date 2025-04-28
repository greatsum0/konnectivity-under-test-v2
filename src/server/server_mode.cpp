// src/server/server_mode.cpp
#include "server_mode.h"
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <__filesystem/operations.h>

#include "cpu_usage_monitor.h"
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
    // create the name of the output file
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);

    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string timestamp = ts.str();

    // build the filename store the test start time
    std::string filename = context.outputPath + "/"
                           + timestamp + "_"
                           + context.name + ".csv";
    // ----------------------------------------------------


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

    // 2) start the port‐level monitor
    PortTrafficMonitor monitor(
        context.measurement.interface,
        context.network.port,
        context.measurement.intervalSec
    );

    /// cpu monitor
    CPUUsageMonitor cpuMonitor(
        context.measurement.intervalSec
    );
    // ----------------------------------------------------

    // 3) start the network monitor
    monitor.start(
        [&](const uint64_t bytesIn, const uint64_t bytesOut) {
            const double interval = context.measurement.intervalSec;
            const double mbpsIn = (bytesIn * 8.0) / (1e6 * interval);
            const double mbpsOut = (bytesOut * 8.0) / (1e6 * interval);
            std::cout << "[METRICS] In: " << bytesIn << " B ("
                    << mbpsIn << " Mbps), Out: "
                    << bytesOut << " B (" << mbpsOut
                    << " Mbps)\n";
        }
    );

    // 4) CPU usage monitor
    cpuMonitor.start(
        [&](double totalCpuPct, double selfCpuPct) {
            auto now = std::chrono::system_clock::now();
            networkMetrics.push_back({now, "cpu_total_pct", totalCpuPct});
            networkMetrics.push_back({now, "cpu_self_pct", selfCpuPct});
        }
    );
    // ----------------------------------------------------

    // 3) run until client disconnects (io.stop is called internally)
    io.run();

    // 4) then stop the monitor
    monitor.stop();
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

    /// append the filename
    utils::PivotCSVWriter writer(filename);

    /// create directories if they do not exist
    std::filesystem::create_directories(context.outputPath);

    writer.write(allMetrics);

    std::cout << "Server shutting down, metrics stopped.\n";
}
