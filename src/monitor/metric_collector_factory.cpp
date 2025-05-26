#include "metric_collector_factory.h"

#include <collector/network_collector.h>

#include "imetric_collector.h"
#include "self_cpu_collector.h"
#include "total_cpu_collector.h"


std::vector<std::unique_ptr<IMetricCollector> >
MetricCollectorFactory::create(const MeasurementConfig &config,
                               std::vector<Metric> &store) {
    const auto &cfg = config;
    std::vector<std::unique_ptr<IMetricCollector> > v;

    for (auto const &name: cfg.metricNames) {
        if (name == "network_logger") {
            v.emplace_back(std::make_unique<NetworkCollector>(store));
        }else if (name == "total_cpu_util") {
            v.emplace_back(
                std::make_unique<TotalCpuCollector>(store)
            );
        } else if (name == "tool_cpu_util") {
            v.emplace_back(
                std::make_unique<SelfCpuCollector>(store)
            );
        }
        // add more mappings here…
    }

    return v;
}
