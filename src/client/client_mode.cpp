#include "client_mode.h"
#include <iostream>
#include "traffic_scheduler.h"
#include <utility>
#include <asio/io_context.hpp>
#include "tcp_client.h"
#include "udp_client.h"
#include "traffic_pattern_factory.h"
#include "metric_collector.h"
#include "metric_scheduler.h"

#include <asio.hpp>
#include <iostream>
#include <memory>
#include <vector>

ClientMode::ClientMode(ClientContext context)
    : context(std::move(context)) {
}

void ClientMode::run() const {
    asio::io_context io;

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

    baseClient->connect();
    std::cout << "[INFO] Connected to "
            << context.network.serverAddress << ":"
            << context.network.serverPort << "\n";

    auto pattern = TrafficPatternFactory::create(context.traffic);

    MetricCollector collector;
    MetricScheduler metricSched(io, collector, context.metricIntervalSec);
    metricSched.start();

    // MeteredClient: reports bytes to collector after each send
    struct MeteredClient : INetworkClient {
        INetworkClient &client;
        MetricCollector &mc;

        MeteredClient(INetworkClient &c, MetricCollector &m)
            : client(c), mc(m) {
        }

        void connect() override { client.connect(); }

        void send(const std::vector<uint8_t> &data) override {
            client.send(data);
            mc.addBytesTransferred(data.size());
        }

        void disconnect() override { client.disconnect(); }
    } metered(*baseClient, collector);

    // 6) Asio traffic scheduler
    TrafficScheduler trafficSched(
        io,
        metered,
        *pattern,
        context.traffic.intervalSec,
        context.traffic.durationSec
    );
    trafficSched.start();
    std::cout << "[INFO] Traffic scheduler started\n";

    //  Set a timer to stop the I/O context after the specified duration
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

    // 8) Event loop
    std::cout << "[INFO] Running I/O context...\n";
    io.run();

    // 9) Close the connection

    std::cout << "[INFO] Disconnecting client...\n";
    baseClient->disconnect();

    // 10) print metrics results
    std::cout << "[INFO] Collected metrics:\n";
    for (auto const &s: collector.getSamples()) {
        std::cout
                << "t=" << s.timestampSec << "s, "
                << "bytes=" << s.bytesTransferred << ", "
                << "Mbps=" << s.throughputMbps << ", "
                << "CPU=" << s.cpuUsagePercent << "%\n";
    }
}

