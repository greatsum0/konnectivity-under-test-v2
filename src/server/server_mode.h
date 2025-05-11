#pragma once

#include <client_context.h>

#include "inetwork_server.h"
#include "metric_store.h"
#include "server_context.h"

class ServerMode {
public:
    explicit ServerMode(ServerContext context);

    std::unique_ptr<INetworkServer> createNetworkServer(asio::io_context &io);

    static std::vector<Metric> combineMetrics(std::vector<Metric> cpuMetrics, std::vector<Metric> networkMetrics);

    std::shared_ptr<asio::io_context> listenTraffic();

    void run();

private:
    ServerContext context;
    ClientContext clientContext;
};
