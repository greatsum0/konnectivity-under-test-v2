// src/server/server_mode.cpp
#include "server_mode.h"
#include <iostream>
#include <memory>
#include <asio.hpp>

#include "port_traffic_monitor.h"
#include "udp_server.h"
#include "tcp_server.h"

ServerMode::ServerMode(ServerContext context)
    : context(std::move(context))
{}

void ServerMode::run() const {
    std::cout << "Running server mode: " << context.name << "\n";

    asio::io_context io;

    // 1) create the server and keep it alive until after io.run()
    std::unique_ptr<TcpServer> tcpServer;
    std::unique_ptr<UdpServer> udpServer;

    if (context.protocol == "tcp") {
        tcpServer = std::make_unique<TcpServer>(io, context);
        tcpServer->start();
    }
    else if (context.protocol == "udp") {
        udpServer = std::make_unique<UdpServer>(io, context);
        udpServer->start();
    }
    else {
        std::cerr << "Unsupported protocol: " << context.protocol << "\n";
        return;
    }

    // 2) start the port‐level monitor
    PortTrafficMonitor monitor(
        context.measurement.interface,
        context.port,
        context.measurement.intervalSec
    );
    monitor.start(
      [&](const uint64_t bytesIn, const uint64_t bytesOut) {
        const double interval = context.measurement.intervalSec;
        const double mbpsIn  = (bytesIn  * 8.0) / (1e6 * interval);
        const double mbpsOut = (bytesOut * 8.0) / (1e6 * interval);
        std::cout << "[METRICS] In: "  << bytesIn  << " B ("
                  << mbpsIn  << " Mbps), Out: "
                  << bytesOut << " B (" << mbpsOut
                  << " Mbps)\n";
      }
    );

    // 3) run until client disconnects (io.stop is called internally)
    io.run();

    // 4) then stop the monitor
    monitor.stop();
    std::cout << "Server shutting down, metrics stopped.\n";
}
