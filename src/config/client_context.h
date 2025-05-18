#pragma once

#include <string>
#include <vector>

#include "measurement_config.h"

struct ClientNetworkConfig {
    std::string protocol;
    std::string serverAddress;
    int serverPort;
    std::string interface;
};

struct ClientTrafficConfig {
    std::string type;           // e.g., "constant", "burst", ...
    double durationSec;
    double intervalSec;

    // Common
    double rateMbps = 0;
    int packetSize = 0;        // in bytes

    // Burst / RandomBurst / PureBurst
    double burstSizeMb = 0;
    double burstDurationSec = 0;
    double burstIntervalSec = 0;

    // Random
    double maxRateMbps = 0;
    double randomSeed = 0;

    // RandomBurst only includes randomSeed
    double minBurstSizeMb = 0;
    double maxBurstSizeMb = 0;
    double minBurstDurationMs = 0;
    double maxBurstDurationMs = 0;
    double minBurstIntervalMs = 0;
    double maxBurstIntervalMs = 0;

    // RampUp / RampDown
    double rampUpTime = 0;
    double rampDownTime = 0;

    // Sinusoidal
    double amplitude = 0;
    double frequency = 0;

};

struct ClientContext {
    std::string name;
    std::string outputPath;

    ClientNetworkConfig network;
    ClientTrafficConfig traffic;
    MeasurementConfig measurement;
};
