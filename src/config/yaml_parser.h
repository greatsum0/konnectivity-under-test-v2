#pragma once

#include <string>
#include <vector>

struct Listener {
    std::string protocol;
    int port;
};

struct ServerConfig {
    std::string name;
    std::string output_path;
    std::vector<Listener> listeners;
    double interval_sec = 1.0;
    std::vector<std::string> enabled_metrics;
};

struct Scenario {
    std::string name;
    std::string output_path;
    std::string protocol;
    std::string server_address;
    int server_port;
    std::string traffic_type;
    int duration_sec;
    double interval_sec = 1.0;
    int rate_mbps;
    int burst_size_mb = 0; // Burst traffic
    double burst_duration_sec = 0; // Burst traffic
    double burst_interval_sec = 0; // Burst traffic
    int ramp_up_time = 0; // Ramp-up traffic
    int ramp_down_time = 0; // Ramp-down traffic
    double amplitude = 0.0; // sinusoidal traffic
    double frequency = 0.0; // sinusoidal traffic
    int random_seed = 0; // Random traffic
    // Random burst traffic
    int min_burst_size_mb = 0;
    int max_burst_size_mb = 0;
    int min_burst_duration_ms = 0;
    int max_burst_duration_ms = 0;
    int min_burst_interval_ms = 0;
    int max_burst_interval_ms = 0;
    int max_rate_mbps = 0; // used in random burst and ramp-up and ramp-down traffic
    std::vector<std::string> enabled_metrics;
};


class YamlParser {
public:
    static ServerConfig parseServerConfig(const std::string &filepath);

    static Scenario parseScenario(const std::string &filepath);
};
