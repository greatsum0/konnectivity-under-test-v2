#include "metric_collector_factory.h"
#include "bytes_transferred_collector.h"
#include "imetric_collector.h"
#include "self_cpu_collector.h"
#include "throughput_collector.h"
#include "total_cpu_collector.h"


std::vector<std::unique_ptr<IMetricCollector> >
MetricCollectorFactory::create(const MeasurementConfig &config,
                               std::vector<Metric> &store) {
    const auto &cfg = config;
    std::vector<std::unique_ptr<IMetricCollector> > v;

    for (auto const &name: cfg.metricNames) {
        if (name == "bytes_transferred") {
            v.emplace_back(
                std::make_unique<BytesTransferredCollector>(store)
            );
        } else if (name == "throughput") {
            v.emplace_back(
                std::make_unique<ThroughputCollector>(
                    store, cfg.intervalSec
                )
            );
        } else if (name == "total_cpu_util") {
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
