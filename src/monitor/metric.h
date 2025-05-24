#pragma once

#include <chrono>
#include <string>

/// One single metric datapoint:
///  - when it happened
///  - what key (e.g. "bytes_in", "cpu_total")
///  - what value
struct Metric {
    std::chrono::system_clock::time_point timestamp;
    std::string key;
    double value;
};
