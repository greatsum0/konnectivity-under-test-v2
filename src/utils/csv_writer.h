#pragma once

#include <string>
#include <vector>
#include "metric_collector.h"

/// Dump MetricSample vector into a CSV file,
/// using only the metrics listed in `metricNames`.
class CsvWriter {
public:
    /// @param filename     Path to write CSV to (overwrites existing).
    /// @param samples      Collected metric samples.
    /// @param metricNames  List of metric names to include (e.g. "throughput", "total_cpu_util").
    static void writeSamples(const std::string &filename,
                             const std::vector<MetricSample> &samples,
                             const std::vector<std::string> &metricNames);
};
