#pragma once

#include <inetwork_client.h>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "client_context.h"
#include "metric_store.h"
#include "tcp_control_channel.h"

class ClientMode {
public:
    explicit ClientMode(ClientContext context);

    static std::vector<Metric> combineMetrics(std::vector<Metric> cpuMetrics, std::vector<Metric> networkMetrics);

    void sendScenarioOverControlPort(asio::ip::tcp::socket socket) const;

    static void sendEndMessageOverControlPort(asio::ip::tcp::socket socket);

    std::unique_ptr<INetworkClient> createNetworkClient(asio::io_context &io) const;

    void startTraffic(const std::shared_ptr<TcpControlChannel> &controlChannel) const;

    void run() const;

private:
    ClientContext context;

};
