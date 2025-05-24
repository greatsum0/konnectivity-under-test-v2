#pragma once

#include <string>
#include <vector>

struct MeasurementConfig {
    double intervalSec;
    std::string interface;
    std::vector<std::string> metricNames;
};
