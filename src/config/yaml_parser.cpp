#include "yaml_parser.h"
#include <yaml-cpp/yaml.h>

// SERVER CONFIG
ServerConfig YamlParser::parseServerConfig(const std::string &filepath) {
    YAML::Node config = YAML::LoadFile(filepath);
    ServerConfig scfg;

    scfg.name = config["server"]["name"].as<std::string>();
    scfg.output_path = config["server"]["output_path"].as<std::string>();

    for (const auto &listener: config["server"]["listeners"]) {
        Listener l;
        l.protocol = listener["protocol"].as<std::string>();
        l.port = listener["port"].as<int>();
        scfg.listeners.push_back(l);
    }

    if (const auto &mc = config["server"]["metric_collection"]) {
        scfg.interval_sec = mc["interval_sec"] ? mc["interval_sec"].as<double>() : 1.0;

        if (mc["metrics"]) {
            for (const auto &metric: mc["metrics"]) {
                scfg.enabled_metrics.push_back(metric.as<std::string>());
            }
        }
    }

    return scfg;
}

// SCENARIO
Scenario YamlParser::parseScenario(const std::string &filepath) {
    YAML::Node root = YAML::LoadFile(filepath);
    YAML::Node scenarioNode = root["scenario"];
    Scenario sc;

    sc.name = scenarioNode["name"].as<std::string>();
    sc.output_path = scenarioNode["output_path"].as<std::string>();
    sc.protocol = scenarioNode["network"]["protocol"].as<std::string>();
    sc.server_address = scenarioNode["network"]["server_address"].as<std::string>();
    sc.server_port = scenarioNode["network"]["server_port"].as<int>();
    sc.traffic_type = scenarioNode["traffic_pattern"]["type"].as<std::string>();
    sc.duration_sec = scenarioNode["traffic_pattern"]["duration_sec"].as<int>();
    sc.interval_sec = scenarioNode["traffic_pattern"]["interval_sec"]
                          ? scenarioNode["traffic_pattern"]["interval_sec"].as<double>()
                          : 1.0;

    const auto& params = scenarioNode["traffic_pattern"]["parameters"];

    if (sc.traffic_type == "burst") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
        sc.burst_size_mb = params["burst_size_mb"].as<int>();
        sc.burst_duration_sec = params["burst_duration_sec"].as<double>();
        sc.burst_interval_sec = params["burst_interval_sec"].as<double>();
    } else if (sc.traffic_type == "pure_burst") {
        sc.burst_size_mb = params["burst_size_mb"].as<int>();
        sc.burst_duration_sec = params["burst_duration_sec"].as<double>();
        sc.burst_interval_sec = params["burst_interval_sec"].as<double>();
    } else if (sc.traffic_type == "random_burst") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
        sc.burst_size_mb = params["burst_size_mb"].as<int>();
        sc.burst_duration_sec = params["burst_duration_sec"].as<double>();
        sc.burst_interval_sec = params["burst_interval_sec"].as<double>();
        sc.random_seed = params["random_seed"].as<int>();
        sc.min_burst_size_mb = params["min_burst_size_mb"] ? params["min_burst_size_mb"].as<int>() : 0;
        sc.max_burst_size_mb = params["max_burst_size_mb"] ? params["max_burst_size_mb"].as<int>() : 0;
        sc.min_burst_duration_ms = params["min_burst_duration_ms"] ? params["min_burst_duration_ms"].as<int>() : 0;
        sc.max_burst_duration_ms = params["max_burst_duration_ms"] ? params["max_burst_duration_ms"].as<int>() : 0;
        sc.min_burst_interval_ms = params["min_burst_interval_ms"] ? params["min_burst_interval_ms"].as<int>() : 0;
        sc.max_burst_interval_ms = params["max_burst_interval_ms"] ? params["max_burst_interval_ms"].as<int>() : 0;
    } else if (sc.traffic_type == "constant") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
    } else if (sc.traffic_type == "ramp_up") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
        sc.ramp_up_time = params["ramp_up_time"].as<int>();
    } else if (sc.traffic_type == "ramp_down") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
        sc.ramp_down_time = params["ramp_down_time"].as<int>();
    } else if (sc.traffic_type == "sinusoidal") {
        sc.rate_mbps = params["rate_mbps"].as<int>();
        sc.amplitude = params["amplitude"].as<double>();
        sc.frequency = params["frequency"].as<double>();
    } else if (sc.traffic_type == "random") {
        sc.random_seed = params["random_seed"].as<int>();
        sc.max_rate_mbps = params["max_rate_mbps"].as<int>();
    }

    if (const auto &mc = scenarioNode["metric_collection"]) {
        if (mc["metrics"]) {
            for (const auto &metric: mc["metrics"]) {
                sc.enabled_metrics.push_back(metric.as<std::string>());
            }
        }
    }

    return sc;
}
