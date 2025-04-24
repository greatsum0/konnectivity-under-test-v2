// src/utils/CsvWriter.cpp
#include "csv_writer.h"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

void CsvWriter::writeSamples(const std::string &filename,
                             const std::vector<MetricSample> &samples,
                             const std::vector<std::string> &metricNames) {
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        throw std::runtime_error("Failed to open CSV file: " + filename);

    // --- HEADER ---
    out << "timestamp_s";
    for (auto const &m: metricNames) {
        if (m == "bytes_transferred") {
            out << ",bytes_transferred";
        } else if (m == "throughput") {
            out << ",throughput_Mbps";
        } else if (m == "tool_cpu_util") {
            out << ",tool_cpu_percent";
        } else if (m == "total_cpu_util") {
            out << ",total_cpu_percent";
        } else {
            out << "," << m;
        }
    }
    out << "\n";

    out << std::fixed << std::setprecision(6);

    // --- ROWS ---
    for (auto const &s: samples) {
        out << s.timestampSec;
        for (auto const &m: metricNames) {
            if (m == "bytes_transferred") {
                out << "," << static_cast<uint64_t>(s.bytesTransferred);
            } else if (m == "throughput") {
                out << "," << s.throughputMbps;
            } else if (m == "tool_cpu_util") {
                out << "," << s.selfCpuUsage;
            } else if (m == "total_cpu_util") {
                out << "," << s.totalCpuUsage;
            } else {
                out << ","; // unknown metric: empty cell
            }
        }
        out << "\n";
    }

    out.close();
}
