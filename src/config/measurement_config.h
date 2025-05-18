#pragma once

#include <string>
#include <vector>

struct MeasurementConfig {
    double intervalSec;
    std::vector<std::string> metricNames;
};
