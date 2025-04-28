#include "client_mode.h"
#include <iostream>
#include <utility>
#include <memory>
#include <vector>
#include <asio.hpp>
#include <port_traffic_monitor.h>
#include <asio/io_context.hpp>
#include <__filesystem/operations.h>
#include "traffic_scheduler.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "traffic_pattern_factory.h"


ClientMode::ClientMode(ClientContext context)
    : context(std::move(context)) {
}

void ClientMode::run() const {
    asio::io_context io;

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

    PortTrafficMonitor monitor(
        context.measurement.interface,
        context.network.serverPort,
        context.measurement.intervalSec
    );

    monitor.start(
      [&](const uint64_t bytesIn, const uint64_t bytesOut) {
        // this is called once per interval_sec
        const double interval = context.measurement.intervalSec;
        const double mbpsIn  = (bytesIn  * 8.0) / (1e6 * interval);
        const double mbpsOut = (bytesOut * 8.0) / (1e6 * interval);

        // for now just log to stdout or store in-memory
        std::cout << "[METRICS] In: "  << bytesIn  << " B ("
                  << mbpsIn  << " Mbps), Out: "
                  << bytesOut << " B (" << mbpsOut
                  << " Mbps)\n";

        // you could also push these into a vector for later:
        // collectedMetrics.emplace_back(bytesIn, bytesOut, mbpsIn, mbpsOut);
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
        io.stop();
    });

    // 7) run!
    std::cout << "[INFO] Running I/O context...\n";
    io.run();

    // 8) teardown
    monitor.stop();
    std::cout << "[INFO] Disconnecting client...\n";
    baseClient->disconnect();
}


