#pragma once

#include <string>
#include <vector>

struct ServerMeasurementConfig {
    double intervalSec;
    std::string interface;
    std::vector<std::string> metricNames;
};

struct ServerNetworkConfig {
    std::string protocol; // "tcp" or "udp"
    int port;
};


struct ServerContext {
    std::string name;
    std::string outputPath;

    ServerMeasurementConfig measurement;
    ServerNetworkConfig network;

};
