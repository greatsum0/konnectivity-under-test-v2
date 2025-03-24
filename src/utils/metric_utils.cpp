#include "metric_utils.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>


void printMetricResults(const std::vector<MetricData>& results, const std::string& label) {
    std::cout << "\n--- " << label << " Throughput Measurement Results ---\n";
    std::cout << std::setw(15) << "Time(s)" << std::setw(20) << "Throughput(Mbps)\n";
    for (const auto& data : results) {
        std::cout << std::setw(15) << data.values.at("time") << std::setw(20) << data.values.at("throughput")
        << std::setw(20) << data.timeSec << "\n";
    }
    std::cout << "-------------------------------------------\n\n";
}
