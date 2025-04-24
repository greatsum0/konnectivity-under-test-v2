#pragma once

#include <vector>
#include <string>
#include "metric_collector.h"

/// Prints metric samples in a neat ASCII table.
class PrettyPrinter {
public:
    /// @param metricNames names in the same format as CsvWriter (e.g. "throughput","tool_cpu_util")
    static void printTable(const std::vector<MetricSample> &samples,
                           const std::vector<std::string> &,
                           double intervalSec);
};
