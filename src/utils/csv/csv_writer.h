#pragma once

#include <string>
#include <vector>
#include "metric.h"

namespace utils {
    /// Writes a collection of Metric entries out as a pivot‐style CSV:
    ///   Timestamp, key1, key2, key3, …
    ///   2025-04-28 15:23:01, 125562500, 0,      1004.5, …
    ///   2025-04-28 15:23:02, 124025000, 0,      992.2,  …
    class PivotCSVWriter {
    public:
        /// @param path  path to write the .csv file
        explicit PivotCSVWriter(std::string path);

        /// Emit the full CSV, overwriting any existing file.
        void write(const std::vector<kut::measurement::Metric> &metrics) const;

    private:
        std::string _path;
    };
} // namespace utils
