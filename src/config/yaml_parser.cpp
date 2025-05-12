#include "yaml_parser.h"
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

// -------------------- PUBLIC CLIENT --------------------

ClientContext YamlParser::parseClientContextFromFile(const std::string &path) {
    YAML::Node root = loadYamlFromFile(path);
    YAML::Node scenario = root["scenario"];

    ClientContext ctx;
    parseGeneralSection(ctx.name, ctx.outputPath, scenario);
    parseClientNetwork(ctx.network, scenario["network"]);
    parseTraffic(ctx.traffic, scenario["traffic_pattern"]);
    parseMeasurementSection(ctx.measurement, scenario["metric_collection"]);
    return ctx;
}

ClientContext YamlParser::parseClientContextFromString(const std::string &yamlText) {
    YAML::Node root = parseYamlFromString(yamlText);
    YAML::Node scenario = root["scenario"];

    ClientContext ctx;
    parseGeneralSection(ctx.name, ctx.outputPath, scenario);
    parseClientNetwork(ctx.network, scenario["network"]);
    parseTraffic(ctx.traffic, scenario["traffic_pattern"]);
    parseMeasurementSection(ctx.measurement, scenario["metric_collection"]);
    return ctx;
}

// -------------------- PUBLIC SERVER --------------------

ServerContext YamlParser::parseServerContextFromFile(const std::string &path) {
    YAML::Node root = loadYamlFromFile(path);
    YAML::Node server = root["server"];

    ServerContext ctx;
    parseGeneralSection(ctx.name, ctx.outputPath, server);
    parseServerNetwork(ctx.network, server["network"]);
    parseMeasurementSection(ctx.measurement, server["metric_collection"]);
    return ctx;
}

ServerContext YamlParser::parseServerContextFromString(const std::string &yamlText) {
    YAML::Node root = parseYamlFromString(yamlText);
    YAML::Node server = root["server"];

    ServerContext ctx;
    parseGeneralSection(ctx.name, ctx.outputPath, server);
    parseServerNetwork(ctx.network, server["network"]);
    parseMeasurementSection(ctx.measurement, server["metric_collection"]);
    return ctx;
}

// -------------------- PRIVATE LOADERS --------------------

YAML::Node YamlParser::loadYamlFromFile(const std::string &path) {
    try {
        return YAML::LoadFile(path);
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Failed to load YAML file: " << e.what() << "\n";
        throw;
    }
}

YAML::Node YamlParser::parseYamlFromString(const std::string &yamlText) {
    try {
        return YAML::Load(yamlText);
    } catch (const YAML::ParserException &e) {
        std::cerr << "[ERROR] Failed to parse YAML string: " << e.what() << "\n";
        throw;
    }
}

// -------------------- SHARED PARSERS --------------------

void YamlParser::parseGeneralSection(std::string &name, std::string &outputPath, const YAML::Node &node) {
    name = node["name"].as<std::string>();
    outputPath = node["output_path"].as<std::string>();
}

void YamlParser::parseMeasurementSection(MeasurementConfig &m, const YAML::Node &node) {
    m.intervalSec = node["interval_sec"].as<double>();
    m.interface = node["interface"].as<std::string>();
    m.metricNames = node["metrics"].as<std::vector<std::string> >();
    if (m.metricNames.empty()) {
        m.metricNames = {"bytes_in", "bytes_out"};
    }
}

// -------------------- CLIENT-SPECIFIC --------------------

void YamlParser::parseClientNetwork(ClientNetworkConfig &net, const YAML::Node &node) {
    net.protocol = node["protocol"].as<std::string>();
    net.serverAddress = node["server_address"].as<std::string>();
    net.serverPort = node["server_port"].as<int>();
}

void YamlParser::parseTraffic(ClientTrafficConfig &t, const YAML::Node &node) {
    t.type = node["type"].as<std::string>();
    t.durationSec = node["duration_sec"].as<double>();
    t.intervalSec = node["interval_sec"].as<double>();

    auto params = node["parameters"];

    if (params["rate_mbps"]) t.rateMbps = params["rate_mbps"].as<double>();
    if (params["packet_size"]) t.packetSize = params["packet_size"].as<int>();

    if (params["burst_size_mb"]) t.burstSizeMb = params["burst_size_mb"].as<double>();
    if (params["burst_duration_sec"]) t.burstDurationSec = params["burst_duration_sec"].as<double>();
    if (params["burst_interval_sec"]) t.burstIntervalSec = params["burst_interval_sec"].as<double>();

    if (params["random_seed"]) t.randomSeed = params["random_seed"].as<double>();
    if (params["min_burst_size_mb"]) t.minBurstSizeMb = params["min_burst_size_mb"].as<double>();
    if (params["max_burst_size_mb"]) t.maxBurstSizeMb = params["max_burst_size_mb"].as<double>();
    if (params["min_burst_duration_ms"]) t.minBurstDurationMs = params["min_burst_duration_ms"].as<double>();
    if (params["max_burst_duration_ms"]) t.maxBurstDurationMs = params["max_burst_duration_ms"].as<double>();
    if (params["min_burst_interval_ms"]) t.minBurstIntervalMs = params["min_burst_interval_ms"].as<double>();
    if (params["max_burst_interval_ms"]) t.maxBurstIntervalMs = params["max_burst_interval_ms"].as<double>();

    if (params["ramp_up_time"]) t.rampUpTime = params["ramp_up_time"].as<double>();
    if (params["ramp_down_time"]) t.rampDownTime = params["ramp_down_time"].as<double>();

    if (params["amplitude"]) t.amplitude = params["amplitude"].as<double>();
    if (params["frequency"]) t.frequency = params["frequency"].as<double>();
    if (params["max_rate_mbps"]) t.maxRateMbps = params["max_rate_mbps"].as<double>();
}

std::string YamlParser::toYamlString(const ClientContext &ctx) {
    YAML::Node node;
    YAML::Node scenario = node["scenario"];

    // General
    scenario["name"] = ctx.name;
    scenario["output_path"] = ctx.outputPath;

    // Network
    scenario["network"]["protocol"] = ctx.network.protocol;
    scenario["network"]["server_address"] = ctx.network.serverAddress;
    scenario["network"]["server_port"] = ctx.network.serverPort;

    // Traffic
    YAML::Node tp = scenario["traffic_pattern"];
    tp["type"] = ctx.traffic.type;
    tp["duration_sec"] = ctx.traffic.durationSec;
    tp["interval_sec"] = ctx.traffic.intervalSec;

    YAML::Node params = tp["parameters"];
    params["rate_mbps"] = ctx.traffic.rateMbps;
    params["packet_size"] = ctx.traffic.packetSize;

    params["burst_size_mb"] = ctx.traffic.burstSizeMb;
    params["burst_duration_sec"] = ctx.traffic.burstDurationSec;
    params["burst_interval_sec"] = ctx.traffic.burstIntervalSec;

    params["random_seed"] = ctx.traffic.randomSeed;
    params["min_burst_size_mb"] = ctx.traffic.minBurstSizeMb;
    params["max_burst_size_mb"] = ctx.traffic.maxBurstSizeMb;
    params["min_burst_duration_ms"] = ctx.traffic.minBurstDurationMs;
    params["max_burst_duration_ms"] = ctx.traffic.maxBurstDurationMs;
    params["min_burst_interval_ms"] = ctx.traffic.minBurstIntervalMs;
    params["max_burst_interval_ms"] = ctx.traffic.maxBurstIntervalMs;

    params["ramp_up_time"] = ctx.traffic.rampUpTime;
    params["ramp_down_time"] = ctx.traffic.rampDownTime;

    params["amplitude"] = ctx.traffic.amplitude;
    params["frequency"] = ctx.traffic.frequency;
    params["max_rate_mbps"] = ctx.traffic.maxRateMbps;

    // Metrics
    YAML::Node m = scenario["metric_collection"];
    m["interval_sec"] = ctx.measurement.intervalSec;
    m["interface"] = ctx.measurement.interface;
    m["metrics"] = ctx.measurement.metricNames;

    std::stringstream ss;
    ss << node;
    return ss.str();
}

// -------------------- SERVER-SPECIFIC --------------------

void YamlParser::parseServerNetwork(ServerNetworkConfig &net, const YAML::Node &node) {
    net.protocol = node["protocol"].as<std::string>();
    net.port = node["port"].as<int>();
}

std::string YamlParser::toYamlString(const ServerContext &ctx) {
    YAML::Node node;
    YAML::Node server = node["server"];

    server["name"] = ctx.name;
    server["output_path"] = ctx.outputPath;

    server["network"]["protocol"] = ctx.network.protocol;
    server["network"]["port"] = ctx.network.port;

    YAML::Node m = server["metric_collection"];
    m["interval_sec"] = ctx.measurement.intervalSec;
    m["interface"] = ctx.measurement.interface;
    m["metrics"] = ctx.measurement.metricNames;

    std::stringstream ss;
    ss << node;
    return ss.str();
}
