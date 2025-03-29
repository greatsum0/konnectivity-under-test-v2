#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>

struct MetricData {
    double timeSec;
    std::unordered_map<std::string, double> values;  // metric name -> value
};

class MetricDataManager {
public:
    MetricDataManager(std::vector<std::string> enabledMetrics);

    void addMetricValue(const std::string& name, double time, double value);
    void printTable() const;
    void saveAsCSV(const std::string& filepath) const;
    bool isMetricEnabled(const std::string& name) const;
    void clear();

private:
    std::vector<std::string> enabledMetrics_;
    std::vector<MetricData> records_;
};
