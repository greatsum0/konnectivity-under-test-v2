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

class ClientMode {
public:
    explicit ClientMode(ClientContext context);

    std::vector<Metric> combineMetrics();

    void writeToCsv(std::vector<Metric> metrics) const;

    std::unique_ptr<INetworkClient> createNetworkClient(asio::io_context &io) const;

    void startTraffic(const std::shared_ptr<TcpControlChannel> &controlChannel);

    void run();


    static void startNetworkMonitor(PortTrafficMonitor &monitor,
                                    const std::vector<std::unique_ptr<IMetricCollector> > &network_collectors);

    static void startCpuMonitor(CPUUsageMonitor &monitor,
                                const std::vector<std::unique_ptr<IMetricCollector> > &cpu_collectors);

private:
    ClientContext context_;
    std::shared_ptr<asio::io_context> ioContext_ = nullptr;
    std::shared_ptr<INetworkClient> baseClient_;
    std::shared_ptr<TcpControlChannel> controlChannel_;

    std::vector<Metric> cpuMetrics_;
    std::vector<Metric> networkMetrics_;

    std::unique_ptr<PortTrafficMonitor> networkMonitor_;
    std::unique_ptr<CPUUsageMonitor> cpuMonitor_;

    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;
    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;

    std::unique_ptr<ITrafficPattern> pattern_;
};
