#pragma once
#include <client_context.h>
#include <controller.h>
#include <cpu_usage_monitor.h>
#include <imetric_collector.h>
#include <metric.h>
#include <port_traffic_monitor.h>
#include <server_context.h>
#include <unified_monitor_controller.h>
#include <boost/asio/io_context.hpp>

class ServerSession {
public:
    explicit ServerSession(ServerContext &context);

    std::shared_ptr<INetworkServer> createNetworkServer(boost::asio::io_context &ioContext, unsigned short port) const;

    [[nodiscard]] int run();

    void pinThread2Core(unsigned coreId);

private:
    ServerContext &context_;

    ClientContext clientContext_;

    std::shared_ptr<boost::asio::io_context> ioContext_;

    std::shared_ptr<Controller> controller_;

    std::shared_ptr<INetworkServer> baseServer_;

    std::shared_ptr<PortTrafficMonitor> networkMonitor_;

    std::shared_ptr<CPUUsageMonitor> cpuMonitor_;

    std::unique_ptr<UnifiedMonitorController> unifiedMonitorController_;

    // IO contexts for isolated monitoring
    std::shared_ptr<boost::asio::io_context> cpuIoContext_;
    std::shared_ptr<boost::asio::io_context> networkIoContext_;

    // Work guards to keep IO contexts alive
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> cpuWorkGuard_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> networkWorkGuard_;

    // Threads dedicated to CPU and Network monitors
    std::vector<std::thread> cpuThreads_;
    std::vector<std::thread> networkThreads_;

    std::vector<std::unique_ptr<IMetricCollector> > cpuCollectors_;

    std::vector<std::unique_ptr<IMetricCollector> > networkCollectors_;

    std::vector<Metric> cpuMetrics_;

    std::vector<Metric> networkMetrics_;

    void startAllMonitors();

    void stopAllMonitors();

    std::vector<Metric> combineMetrics();

    void writeToCsv(const std::vector<Metric> &metrics) const;
};
