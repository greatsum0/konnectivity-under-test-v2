#include "client_mode.h"
#include <iostream>
#include <utility>
#include <memory>
#include <vector>
#include <asio.hpp>
#include <port_traffic_monitor.h>
#include <cpu_usage_monitor.h>            // <— add CPU monitor
#include <asio/io_context.hpp>
#include <__filesystem/operations.h>

#include "metric_collector_factory.h"
#include "metric_store.h"
#include "traffic_scheduler.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "traffic_pattern_factory.h"
#include "utils/pretty_printer.h"
#include "utils/csv_writer.h"

ClientMode::ClientMode(ClientContext context)
    : context(std::move(context)) {
}

void ClientMode::run() const {
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

    asio::io_context io;

    /// two different metric stores because of race conditions, to save time on lock mechanism. merge at the end.
    std::vector<Metric> cpuMetrics;
    std::vector<Metric> networkMetrics;

    // 1) pick TCP or UDP
    std::unique_ptr<INetworkClient> baseClient;
    if (context.network.protocol == "tcp") {
        baseClient = std::make_unique<TcpClient>(io, context);
    } else if (context.network.protocol == "udp") {
        baseClient = std::make_unique<UdpClient>(io, context);
    } else {
        std::cerr << "[ERROR] Unsupported protocol: "
                << context.network.protocol << "\n";
        return;
    }

    // 2) connect
    baseClient->connect();
    std::cout << "[INFO] Connected to "
            << context.network.serverAddress << ":"
            << context.network.serverPort << "\n";

    // 3) port‐level traffic monitor
    PortTrafficMonitor networkMonitor(
        context.measurement.interface,
        context.network.serverPort,
        context.measurement.intervalSec
    );
    /// cpu monitor
    CPUUsageMonitor cpuMonitor(
        context.measurement.intervalSec
    );


    // 2) network callback
    auto network_collectors = MetricCollectorFactory::create(context, networkMetrics);
    networkMonitor.start(
      [&](auto in, auto out){
        for (auto& c : network_collectors)
            c->onNetworkSample(std::chrono::system_clock::now(), in, out);
        }
      );

    // 3) CPU callback
    auto cpu_collectors = MetricCollectorFactory::create(context, cpuMetrics);
    cpuMonitor.start(
      [&](auto total, auto self){
        for (auto& c : cpu_collectors)
            c->onCpuSample(std::chrono::system_clock::now(), total, self);
        }
      );


    std::unique_ptr<ITrafficPattern> pattern = TrafficPatternFactory::create(context.traffic);


    TrafficScheduler trafficSched(
        io,
        *baseClient,
        context,
        *pattern
    );
    trafficSched.start();
    std::cout << "[INFO] Traffic scheduler started\n";

    // 6) stop IO loop after durationSec
    asio::steady_timer exitTimer(io);
    exitTimer.expires_after(
        std::chrono::duration_cast<asio::steady_timer::duration>(
            std::chrono::duration<double>(context.traffic.durationSec)
        )
    );
    exitTimer.async_wait([&](const asio::error_code &) {
        std::cout << "[INFO] Duration elapsed, stopping I/O context\n";
        // send an END message to the server only if the client and server are UDP
        // Because TCP is a stream protocol, the client and server will know when the connection is closed
        // But for UDP, we need to send a message to the server to indicate that the client is done
        // Because server needs to know when to stop receiving data and to save the log file.
        if (context.network.protocol == "udp") {
            std::cout << "[INFO] Sending END message to server...\n";
            std::string marker = "END";
            std::vector<uint8_t> endMsg(marker.begin(), marker.end());
            baseClient->send(endMsg);
        }
        io.stop();
    });

    // 7) run!
    std::cout << "[INFO] Running I/O context...\n";
    io.run();

    // 8) teardown monitors & client
    networkMonitor.stop();
    cpuMonitor.stop();
    std::cout << "[INFO] Disconnecting client...\n";
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

    baseClient->disconnect();
}
