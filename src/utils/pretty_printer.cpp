#include "pretty_printer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

void PrettyPrinter::printTable(const std::vector<MetricSample> &samples,
                               const std::vector<std::string> &metricNames,
                               double intervalSec) {
  struct Col {
    std::string header;
    size_t width;
  };
  std::vector<Col> cols;

  // Always include timestamp
  bool useMs = intervalSec < 1.0;
  const char *timeLabel = useMs ? "t[ms]" : "t[s]";
  cols.push_back({timeLabel, std::max<size_t>(5, std::string(timeLabel).size())});

  // map metricNames → headers
  for (auto const &m: metricNames) {
    if (m == "bytes_transferred") cols.push_back({"bytes", 6});
    else if (m == "throughput") cols.push_back({"Mbps", 4});
    else if (m == "tool_cpu_util") cols.push_back({"toolCPU%", 8});
    else if (m == "total_cpu_util") cols.push_back({"totalCPU%", 9});
    else cols.push_back({m, m.size()});
  }

  // compute column widths by scanning data
  for (auto const &sample: samples) {
    size_t col = 0;
    // timestamp
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(3) << sample.timestampSec;
      cols[col].width = std::max(cols[col].width, ss.str().size());
    }
    // other metrics
    for (auto const &mn: metricNames) {
      ++col;
      std::ostringstream ss;
      if (mn == "bytes_transferred") ss << static_cast<uint64_t>(sample.bytesTransferred);
      else if (mn == "throughput") ss << std::fixed << std::setprecision(3) << sample.throughputMbps;
      else if (mn == "tool_cpu_util") ss << std::fixed << std::setprecision(1) << sample.selfCpuUsage;
      else if (mn == "total_cpu_util") ss << std::fixed << std::setprecision(1) << sample.totalCpuUsage;
      else ss << "-";
      cols[col].width = std::max(cols[col].width, ss.str().size());
    }
  }

  // print header separator
  auto printSep = [&]() {
    std::cout << '+';
    for (auto &c: cols) {
      std::cout << std::string(c.width + 2, '-') << '+';
    }
    std::cout << "\n";
  };

  // top border
  printSep();

  // header row
  std::cout << "| ";
  for (auto &c: cols) {
    std::cout << std::setw(c.width) << c.header << " | ";
  }
  std::cout << "\n";

  // header/data separator
  printSep();

  // each data row
  for (auto const &sample: samples) {
    size_t col = 0;
    std::cout << "| " << std::setw(cols[col].width)
        << std::fixed << std::setprecision(3)
        << sample.timestampSec << " | ";
    for (auto const &mn: metricNames) {
      ++col;
      std::cout << std::setw(cols[col].width);
      if (mn == "bytes_transferred") std::cout << static_cast<uint64_t>(sample.bytesTransferred);
      else if (mn == "throughput") std::cout << std::fixed << std::setprecision(3) << sample.throughputMbps;
      else if (mn == "tool_cpu_util") std::cout << std::fixed << std::setprecision(1) << sample.selfCpuUsage;
      else if (mn == "total_cpu_util") std::cout << std::fixed << std::setprecision(1) << sample.totalCpuUsage;
      else std::cout << "-";
      std::cout << " | ";
    }
    std::cout << "\n";
  }

  // bottom border
  printSep();
}
