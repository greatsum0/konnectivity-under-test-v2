#pragma once

#include <string>
#include <vector>

struct ServerMeasurementConfig {
    double intervalSec;
    std::string interface;
    std::vector<std::string> metricNames;
};

struct ServerContext {
    std::string name;
    std::string outputPath;

    std::string protocol; // "tcp" or "udp"
    int port;

    ServerMeasurementConfig measurement;

};
