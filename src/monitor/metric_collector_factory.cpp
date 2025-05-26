#include "metric_collector_factory.h"

#include <collector/network_collector.h>

#include "imetric_collector.h"
#include "system_collector.h"


std::vector<std::unique_ptr<IMetricCollector> >
MetricCollectorFactory::create(const MeasurementConfig &config,
                               std::vector<Metric> &store) {
    const auto &cfg = config;
    std::vector<std::unique_ptr<IMetricCollector> > v;

    for (auto const &name: cfg.metricNames) {
        if (name == "network_logger") {
            v.emplace_back(std::make_unique<NetworkCollector>(store));
        }if (name == "cpu_logger") {
            v.emplace_back(std::make_unique<SystemCollector>(store));
        }
        // add more mappings here…
    }

    return v;
}
