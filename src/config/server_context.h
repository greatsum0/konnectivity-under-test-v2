#pragma once

#include <string>
#include <vector>

struct ServerContext {
    std::string name;
    std::string outputPath;

    std::string protocol; // "tcp" or "udp"
    int port;

    double metricIntervalSec;
    std::vector<std::string> metricNames;
};
