#pragma once

#include <vector>
#include "metric_store.h"
#include "imetric_collector.h"
#include "measurement_config.h"

/// Builds the requested collectors from the user’s config.
class MetricCollectorFactory {
public:
    static std::vector<std::unique_ptr<IMetricCollector> >
    create(const MeasurementConfig &config,
           std::vector<Metric> &store);
};
