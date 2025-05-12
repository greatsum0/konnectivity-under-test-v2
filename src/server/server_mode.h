#pragma once

#include <client_context.h>

#include "cpu_usage_monitor.h"
#include "imetric_collector.h"
#include "inetwork_server.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "server_context.h"
#include "tcp_control_channel.h"

class ServerMode {
public:
    explicit ServerMode(ServerContext context);

    std::unique_ptr<INetworkServer> createNetworkServer(asio::io_context &io);

    std::vector<Metric> combineMetrics();

    void writeToCsv(std::vector<Metric> metrics) const;

    static void startNetworkMonitor(PortTrafficMonitor &monitor,
                                    const std::vector<std::unique_ptr<IMetricCollector> > &network_collectors);

    static void startCpuMonitor(CPUUsageMonitor &monitor,
                                const std::vector<std::unique_ptr<IMetricCollector> > &cpu_collectors);

    std::shared_ptr<asio::io_context> listenTraffic();

    void run();

private:
    ServerContext context;
    ClientContext clientContext;

    std::shared_ptr<INetworkServer> baseServer;
    std::shared_ptr<TcpControlChannel> controlChannel;
    std::shared_ptr<asio::io_context> ioContext = nullptr;

    std::vector<Metric> cpuMetrics;
    std::vector<Metric> networkMetrics;

    std::unique_ptr<PortTrafficMonitor> networkMonitor_;
    std::unique_ptr<CPUUsageMonitor> cpuMonitor_;

    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;
    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;
};
