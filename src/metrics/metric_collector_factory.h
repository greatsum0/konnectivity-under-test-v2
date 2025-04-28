#pragma once

#include <vector>
#include <memory>
#include "metric_store.h"
#include "client_context.h"  // for context.metric_collection
#include "imetric_collector.h"

/// Builds the requested collectors from the user’s config.
class MetricCollectorFactory {
public:
    static std::vector<std::unique_ptr<IMetricCollector> >
    create(const ClientContext &context,
           std::vector<Metric> &store);
};
