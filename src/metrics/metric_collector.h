#pragma once

#include <asio.hpp>
#include <vector>
#include <string>
#include <memory>

#include "metric_data_manager.h"

class MetricCollector {
public:

    MetricCollector(asio::io_context &io_context, double interval_sec, const std::vector<std::string> &enabledMetrics,
                    std::string outputCsvPath);

    void start();
    void stop();
    void addBytesTransferred(int bytes);
    [[nodiscard]] std::shared_ptr<MetricDataManager> getManager() const;
    void reset();

private:
    void measure();
    [[nodiscard]] bool isMetricEnabled(const std::string& name) const;

    double getTotalCPUUsage();

    double getNormalizedProcessCPUUsage() const;

    [[nodiscard]] double getProcessCPUUsage() const;

    asio::steady_timer timer_;
    double interval_sec_;
    int bytesTransferred_;
    double elapsed_sec_;
    std::vector<std::string> enabledMetrics_;
    std::string outputCsvPath_;

    MetricDataManager dataManager_;
};
