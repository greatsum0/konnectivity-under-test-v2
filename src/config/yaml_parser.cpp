#include "yaml_parser.h"

#include <client_context.h>
#include <server_context.h>
#include <yaml-cpp/yaml.h>

ServerContext YamlParser::parseServerContext(const std::string &path) {
    YAML::Node root = YAML::LoadFile(path);
    YAML::Node server = root["server"];

    ServerContext context;

    // General information
    context.name = server["name"].as<std::string>();
    context.outputPath = server["output_path"].as<std::string>();
    context.protocol = server["protocol"].as<std::string>();
    context.port = server["port"].as<int>();

    // Metric collection
    auto metrics = server["metric_collection"];
    context.measurement.intervalSec = metrics["interval_sec"].as<double>();
    context.measurement.interface = metrics["interface"].as<std::string>();
    context.measurement.metricNames = metrics["metrics"].as<std::vector<std::string> >();
    if (context.measurement.metricNames.empty()) {
        context.measurement.metricNames = {"bytes_in", "bytes_out"};
    }

    return context;
}


ClientContext YamlParser::parseClientContext(const std::string &path) {
    YAML::Node root = YAML::LoadFile(path);
    YAML::Node scenario = root["scenario"];

    ClientContext context;

    // General information
    context.name = scenario["name"].as<std::string>();
    context.outputPath = scenario["output_path"].as<std::string>();

    // network setup
    auto net = scenario["network"];
    context.network.protocol = net["protocol"].as<std::string>();
    context.network.serverAddress = net["server_address"].as<std::string>();
    context.network.serverPort = net["server_port"].as<int>();

    // Traffic pattern
    auto traffic = scenario["traffic_pattern"];
    context.traffic.type = traffic["type"].as<std::string>();
    context.traffic.durationSec = traffic["duration_sec"].as<double>();
    context.traffic.intervalSec = traffic["interval_sec"].as<double>();

    // Parameters
    auto params = traffic["parameters"];

    if (params["rate_mbps"]) context.traffic.rateMbps = params["rate_mbps"].as<double>();

    if (params["packet_size"]) context.traffic.packetSize = params["packet_size"].as<int>();

    if (params["burst_size_mb"]) context.traffic.burstSizeMb = params["burst_size_mb"].as<double>();
    if (params["burst_duration_sec"]) context.traffic.burstDurationSec = params["burst_duration_sec"].as<double>();
    if (params["burst_interval_sec"]) context.traffic.burstIntervalSec = params["burst_interval_sec"].as<double>();

    if (params["random_seed"]) context.traffic.randomSeed = params["random_seed"].as<double>();
    if (params["min_burst_size_mb"]) context.traffic.minBurstSizeMb = params["min_burst_size_mb"].as<double>();
    if (params["max_burst_size_mb"]) context.traffic.maxBurstSizeMb = params["max_burst_size_mb"].as<double>();
    if (params["min_burst_duration_ms"]) context.traffic.minBurstDurationMs = params["min_burst_duration_ms"].as<
                                             double>();
    if (params["max_burst_duration_ms"]) context.traffic.maxBurstDurationMs = params["max_burst_duration_ms"].as<
                                             double>();
    if (params["min_burst_interval_ms"]) context.traffic.minBurstIntervalMs = params["min_burst_interval_ms"].as<
                                             double>();
    if (params["max_burst_interval_ms"]) context.traffic.maxBurstIntervalMs = params["max_burst_interval_ms"].as<
                                             double>();

    if (params["ramp_up_time"]) context.traffic.rampUpTime = params["ramp_up_time"].as<double>();
    if (params["ramp_down_time"]) context.traffic.rampDownTime = params["ramp_down_time"].as<double>();

    if (params["amplitude"]) context.traffic.amplitude = params["amplitude"].as<double>();
    if (params["frequency"]) context.traffic.frequency = params["frequency"].as<double>();

    if (params["max_rate_mbps"]) context.traffic.maxRateMbps = params["max_rate_mbps"].as<double>();

    // Metric collection
    auto metrics = scenario["metric_collection"];

    context.measurement.intervalSec = metrics["interval_sec"].as<double>();
    context.measurement.interface = metrics["interface"].as<std::string>();
    context.measurement.metricNames = metrics["metrics"].as<std::vector<std::string> >();
    if (context.measurement.metricNames.empty()) {
        context.measurement.metricNames = {"bytes_in", "bytes_out"};
    }


    return context;
}

