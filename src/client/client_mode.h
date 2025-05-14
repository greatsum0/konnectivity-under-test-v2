#pragma once

#include <inetwork_client.h>
#include <itraffic_pattern.h>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "client_context.h"
#include "cpu_usage_monitor.h"
#include "imetric_collector.h"
#include "metric_store.h"
#include "port_traffic_monitor.h"
#include "tcp_control_channel.h"
#include "unified_monitor_controller.h"

class ClientMode {
public:
    explicit ClientMode(ClientContext context);

    std::vector<Metric> combineMetrics();

    void writeToCsv(std::vector<Metric> metrics) const;

    std::unique_ptr<INetworkClient> createNetworkClient(asio::io_context &io) const;

    void startTraffic();

    void run();

    void startAllMonitors(const std::chrono::system_clock::time_point &referenceTime);

private:
    ClientContext context_;
    std::shared_ptr<asio::io_context> ioContext_ = nullptr;
    std::shared_ptr<INetworkClient> baseClient_;
    std::shared_ptr<TcpControlChannel> controlChannel_;

    std::vector<Metric> cpuMetrics_;
    std::vector<Metric> networkMetrics_;

    std::shared_ptr<PortTrafficMonitor> networkMonitor_;
    std::shared_ptr<CPUUsageMonitor> cpuMonitor_;

    std::unique_ptr<UnifiedMonitorController> unifiedMonitorController_;
    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;
    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;

    std::unique_ptr<ITrafficPattern> pattern_;
};
