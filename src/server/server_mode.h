// server_mode.h
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <asio/io_context.hpp>
#include <client_context.h>
#include <server_context.h>
#include <inetwork_server.h>
#include <imetric_collector.h>
#include <cpu_usage_monitor.h>
#include <port_traffic_monitor.h>
#include <unified_monitor_controller.h>
#include "tcp_control_channel.h"
#include "metric_collector_factory.h"
#include "utils/csv_writer.h"
#include "utils/pretty_printer.h"

#define SESSION_LOG_INFO(msg) \
LOG_INFO("[SESSION] " << msg)

#define SESSION_LOG_WARN(msg) \
LOG_WARN("[SESSION] " << msg)

#define SESSION_LOG_ERROR(msg) \
LOG_ERROR("[SESSION] " << msg)

constexpr uint16_t CONTROL_PORT = 6070;

class ServerMode {
public:
    explicit ServerMode(ServerContext context);

    void run();

private:
    std::unique_ptr<INetworkServer> createNetworkServer(asio::io_context &io);

    std::vector<Metric> combineMetrics();

    void writeToCsv(const std::vector<Metric> &metrics) const;

    void startAllMonitors();

    void stopAllMonitors() const;

    ServerContext context_;
    ClientContext clientContext_;
    bool isReset_ = false;

    std::shared_ptr<asio::io_context> ioContext_;
    std::shared_ptr<TcpControlChannel> controlChannel_;
    std::unique_ptr<INetworkServer> baseServer_;
    std::unique_ptr<UnifiedMonitorController> unifiedMonitorController_;

    std::shared_ptr<PortTrafficMonitor> networkMonitor_;
    std::shared_ptr<CPUUsageMonitor> cpuMonitor_;

    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;
    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;
    std::vector<Metric> networkMetrics_;
    std::vector<Metric> cpuMetrics_;
};
