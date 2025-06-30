#pragma once

#include <vector>
#include "metric.h"

namespace utils {
    /// Pretty-prints metrics in pivot form:
    ///   Timestamp | key1 | key2 | key3 | …
    ///   2025-04-28… | 125M | 1004.5 | 5 | …
    class PrettyPrinter {
    public:
        static void printPivotTable(const std::vector<kut::measurement::Metric> &metrics);
    };
} // namespace utils
