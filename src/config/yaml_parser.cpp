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

// SCENARIOS
std::vector<Scenario> YamlParser::parseScenarios(const std::string &filepath) {
    YAML::Node root = YAML::LoadFile(filepath);
    std::vector<Scenario> scenarios;

    for (const auto &scenarioNode: root["scenarios"]) {
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

        if (sc.traffic_type == "burst") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
            sc.burst_size_mb = scenarioNode["traffic_pattern"]["parameters"]["burst_size_mb"].as<int>();
            sc.burst_duration_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_duration_sec"].as<double>();
            sc.burst_interval_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_interval_sec"].as<double>();
        } else if (sc.traffic_type == "pure_burst") {
            sc.burst_size_mb = scenarioNode["traffic_pattern"]["parameters"]["burst_size_mb"].as<int>();
            sc.burst_duration_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_duration_sec"].as<double>();
            sc.burst_interval_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_interval_sec"].as<double>();
        } else if (sc.traffic_type == "random_burst") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
            sc.burst_size_mb = scenarioNode["traffic_pattern"]["parameters"]["burst_size_mb"].as<int>();
            sc.burst_duration_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_duration_sec"].as<double>();
            sc.burst_interval_sec = scenarioNode["traffic_pattern"]["parameters"]["burst_interval_sec"].as<double>();
            sc.random_seed = scenarioNode["traffic_pattern"]["parameters"]["random_seed"].as<int>();
            sc.min_burst_size_mb = scenarioNode["traffic_pattern"]["parameters"]["min_burst_size_mb"]
                                       ? scenarioNode["traffic_pattern"]["parameters"]["min_burst_size_mb"].as<int>()
                                       : 0;
            sc.max_burst_size_mb = scenarioNode["traffic_pattern"]["parameters"]["max_burst_size_mb"]
                                       ? scenarioNode["traffic_pattern"]["parameters"]["max_burst_size_mb"].as<int>()
                                       : 0;
            sc.min_burst_duration_ms = scenarioNode["traffic_pattern"]["parameters"]["min_burst_duration_ms"]
                                           ? scenarioNode["traffic_pattern"]["parameters"]["min_burst_duration_ms"].as<
                                               int>()
                                           : 0;
            sc.max_burst_duration_ms = scenarioNode["traffic_pattern"]["parameters"]["max_burst_duration_ms"]
                                           ? scenarioNode["traffic_pattern"]["parameters"]["max_burst_duration_ms"].as<
                                               int>()
                                           : 0;
            sc.min_burst_interval_ms = scenarioNode["traffic_pattern"]["parameters"]["min_burst_interval_ms"]
                                           ? scenarioNode["traffic_pattern"]["parameters"]["min_burst_interval_ms"].as<
                                               int>()
                                           : 0;
            sc.max_burst_interval_ms = scenarioNode["traffic_pattern"]["parameters"]["max_burst_interval_ms"]
                                           ? scenarioNode["traffic_pattern"]["parameters"]["max_burst_interval_ms"].as<
                                               int>()
                                           : 0;
        } else if (sc.traffic_type == "constant") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
        } else if (sc.traffic_type == "ramp_up") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
            sc.ramp_up_time = scenarioNode["traffic_pattern"]["parameters"]["ramp_up_time"].as<int>();
        } else if (sc.traffic_type == "ramp_down") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
            sc.ramp_down_time = scenarioNode["traffic_pattern"]["parameters"]["ramp_down_time"].as<int>();
        } else if (sc.traffic_type == "sinusoidal") {
            sc.rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["rate_mbps"].as<int>();
            sc.amplitude = scenarioNode["traffic_pattern"]["parameters"]["amplitude"].as<double>();
            sc.frequency = scenarioNode["traffic_pattern"]["parameters"]["frequency"].as<double>();
        } else if (sc.traffic_type == "random") {
            sc.random_seed = scenarioNode["traffic_pattern"]["parameters"]["random_seed"].as<int>();
            sc.max_rate_mbps = scenarioNode["traffic_pattern"]["parameters"]["max_rate_mbps"].as<int>();
        }

        if (const auto &mc = scenarioNode["metric_collection"]) {
            if (mc["metrics"]) {
                for (const auto &metric: mc["metrics"]) {
                    sc.enabled_metrics.push_back(metric.as<std::string>());
                }
            }
        }

        scenarios.push_back(sc);
    }

    return scenarios;
}
