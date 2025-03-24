#pragma once

#include "yaml_parser.h"
#include "metric_collector.h"
#include <asio.hpp>
#include <memory>
#include <string>

struct ServerContext {
    asio::io_context &ioContext;
    ServerConfig config;
    std::shared_ptr<MetricCollector> metricCollector;

    explicit ServerContext(asio::io_context &ctx, const ServerConfig &cfg)
        : ioContext(ctx),
          config(cfg),
          metricCollector(std::make_shared<MetricCollector>(ctx, cfg.interval_sec, cfg.enabled_metrics, cfg.output_path)) {
    }

    [[nodiscard]] int getPortForProtocol(const std::string &protocol) const {
        for (const auto &listener: config.listeners) {
            if (listener.protocol == protocol) {
                return listener.port;
            }
        }
        throw std::runtime_error("Port not found for protocol: " + protocol);
    }
};
