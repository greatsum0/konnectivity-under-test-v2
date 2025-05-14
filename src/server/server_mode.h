#pragma once

#include <client_context.h>

#include "cpu_usage_monitor.h"
#include "imetric_collector.h"
#include "inetwork_server.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "server_context.h"
#include "tcp_control_channel.h"
#include "unified_monitor_controller.h"

class ServerMode {
public:
    explicit ServerMode(ServerContext context);

    std::unique_ptr<INetworkServer> createNetworkServer(asio::io_context &io);

    std::vector<Metric> combineMetrics();

    void writeToCsv(std::vector<Metric> metrics) const;

    void startAllMonitors(const std::chrono::system_clock::time_point &referenceTime);

    std::shared_ptr<asio::io_context> listenTraffic();

    void run();

private:
    ServerContext context_;
    ClientContext clientContext_;
    bool isReset = false;
    std::shared_ptr<INetworkServer> baseServer_;
    std::shared_ptr<TcpControlChannel> controlChannel_;
    std::shared_ptr<asio::io_context> ioContext_ = nullptr;
    std::unique_ptr<UnifiedMonitorController> unifiedMonitorController_;

    std::vector<Metric> cpuMetrics_;
    std::vector<Metric> networkMetrics_;

    std::shared_ptr<PortTrafficMonitor> networkMonitor_;
    std::shared_ptr<CPUUsageMonitor> cpuMonitor_;

    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;
    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;
};
