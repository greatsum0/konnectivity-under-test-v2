#pragma once

#include "yaml_parser.h"
#include "metric_collector.h"
#include <asio.hpp>
#include <memory>

struct ClientContext {
    asio::io_context &ioContext;
    Scenario scenario;
    std::shared_ptr<MetricCollector> metricCollector;

    explicit ClientContext(asio::io_context &ctx, const Scenario &sc)
        : ioContext(ctx),
          scenario(sc),
          metricCollector(std::make_shared<MetricCollector>(ctx, sc.interval_sec, sc.enabled_metrics, sc.output_path)) {
    }
    static ClientContext fromScenario(asio::io_context& ctx, const Scenario& sc) {
        return ClientContext(ctx, sc);
    }

    [[nodiscard]] std::string protocol() const { return scenario.protocol; }
    [[nodiscard]] std::string serverAddress() const { return scenario.server_address; }
    [[nodiscard]] int serverPort() const { return scenario.server_port; }
    [[nodiscard]] int rateMbps() const { return scenario.rate_mbps; }
    [[nodiscard]] int durationSec() const { return scenario.duration_sec; }
    [[nodiscard]] double intervalSec() const { return scenario.interval_sec; }
};
