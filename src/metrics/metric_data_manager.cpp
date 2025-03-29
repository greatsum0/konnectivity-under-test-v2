#include "metric_data_manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

MetricDataManager::MetricDataManager(std::vector<std::string> enabledMetrics)
    : enabledMetrics_(std::move(enabledMetrics)) {}

bool MetricDataManager::isMetricEnabled(const std::string& name) const {
    return std::find(enabledMetrics_.begin(), enabledMetrics_.end(), name) != enabledMetrics_.end();
}

void MetricDataManager::addMetricValue(const std::string& name, double time, double value) {
    if (!isMetricEnabled(name)) return;

    if (records_.empty() || records_.back().timeSec != time) {
        records_.emplace_back(MetricData{time, {}});
    }

    records_.back().values[name] = value;
}

void MetricDataManager::printTable() const {
    // Header
    std::cout << std::setw(10) << "Time(s)";
    for (const auto& metric : enabledMetrics_) {
        std::cout << std::setw(20) << metric;
    }
    std::cout << "\n";

    // Rows
    for (const auto& record : records_) {
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << record.timeSec;
        for (const auto& metric : enabledMetrics_) {
            double val = record.values.count(metric) ? record.values.at(metric) : 0.0;
            std::cout << std::setw(20) << std::fixed << std::setprecision(2) << val;
        }
        std::cout << "\n";
    }
}

void MetricDataManager::saveAsCSV(const std::string& filepath) const {
    // create the directory if it doesn't exist
    try {
        fs::path dir = fs::path(filepath).parent_path();
        if (!dir.empty() && !fs::exists(dir)) {
            fs::create_directories(dir);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[Error] Failed to create directory for " << filepath << ": " << e.what() << "\n";
        return;
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "[Error] Could not open file: " << filepath << "\n";
        return;
    }

    // Header
    out << "time";
    for (const auto& metric : enabledMetrics_) {
        out << "," << metric;
    }
    out << "\n";

    // Rows
    for (const auto& record : records_) {
        out << record.timeSec;
        for (const auto& metric : enabledMetrics_) {
            double val = record.values.count(metric) ? record.values.at(metric) : 0.0;
            out << "," << val;
        }
        out << "\n";
    }

    out.close();
    std::cout << "[INFO] Data successfully saved to " << filepath << "\n";
}
