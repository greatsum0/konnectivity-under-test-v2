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
#include <__filesystem/operations.h>

#include "utils/csv_writer.h"
#include "utils/pretty_printer.h"

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

    // 3) metrics
    // MetricCollector collector(context.metricNames);
    // MetricScheduler metricScheduler(io, collector, context.metricIntervalSec);
    // metricScheduler.start();

    // // 4) wrap client to count bytes sent
    // struct MeteredClient : INetworkClient {
    //     INetworkClient &client;
    //     MetricCollector &mc;
    //     MeteredClient(INetworkClient &c, MetricCollector &m)
    //       : client(c), mc(m) {}
    //     void connect() override            { client.connect(); }
    //     void send(const std::vector<uint8_t> &data) override {
    //         client.send(data);
    //         mc.addBytesTransferred(data.size());
    //     }
    //     void disconnect() override         { client.disconnect(); }
    // } metered(*baseClient, collector);

    // 5) constant-rate scheduler

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
    std::cout << "[INFO] Disconnecting client...\n";
    baseClient->disconnect();
}


