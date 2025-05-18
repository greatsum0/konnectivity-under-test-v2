#pragma once

#include <string>
#include <vector>

#include "measurement_config.h"

struct ServerNetworkConfig {
    std::string protocol; // "tcp" or "udp"
    int port;
    std::string interface;
};


struct ServerContext {
    std::string name;
    std::string outputPath;

    MeasurementConfig measurement;
    ServerNetworkConfig network;

};
