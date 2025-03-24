#pragma once

#include <asio.hpp>
#include <string>

#include "metric_collector.h"
#include "metric_data_manager.h"

void scheduleMetricCollectionSummary(asio::io_context &ioContext,
                                     const std::shared_ptr<MetricCollector> &metricCollector,
                                     double duration_sec,
                                     const std::string &label = "Server");
void printMetricResults(const std::vector<MetricData>& results, const std::string& label = "Results");
