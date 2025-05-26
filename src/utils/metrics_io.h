#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <format>
#include "metric.h"
#include "logger.h"
#include "csv_writer.h"

namespace utils {
    inline void writeMetricsToCsv(const std::vector<Metric> &metrics,
                                  const std::string &outputDir,
                                  const std::string &scenarioName,
                                  const std::string &protocol) {
        try {
            std::filesystem::create_directories(outputDir);
        } catch (const std::filesystem::filesystem_error &e) {
            Logger::instance().error(
                "Could not create output directory '" + outputDir +
                "': " + e.what() + "; skipping CSV write");
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const auto ts = std::format("{:%Y%m%d_%H%M%S}", now);
        const auto filename = outputDir + "/" + ts + "_" + protocol + "_" + scenarioName + ".csv";

        try {
            const utils::PivotCSVWriter writer(filename);
            writer.write(metrics);
            Logger::instance().info("Metrics written to " + filename);
        } catch (const std::exception &e) {
            Logger::instance().error(
                "Failed to write metrics to CSV '" + filename + "': " + e.what());
        }
    }
} // namespace utils
