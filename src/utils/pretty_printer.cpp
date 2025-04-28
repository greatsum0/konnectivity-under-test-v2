#include "utils/pretty_printer.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <chrono>
#include <ctime>
#include <sstream>

namespace utils {
    void PrettyPrinter::printPivotTable(const std::vector<Metric> &metrics) {
        // 1) Build sets of unique timestamps (as strings) & keys
        std::map<std::string, std::map<std::string, std::string> > table;
        std::set<std::string> keys;

        for (auto const &m: metrics) {
            // format timestamp
            auto tt = std::chrono::system_clock::to_time_t(m.timestamp);
            std::tm tm = *std::localtime(&tt);
            std::ostringstream tos;
            tos << std::put_time(&tm, "%F %T");
            std::string ts = tos.str();

            // record key and value
            keys.insert(m.key);
            table[ts][m.key] = std::to_string(m.value);
        }

        // 2) Determine column widths
        std::string col0 = "Timestamp";
        std::map<std::string, size_t> colWidth;
        colWidth[col0] = col0.size();
        for (auto const &k: keys) {
            colWidth[k] = k.size();
        }
        for (auto const &[ts,row]: table) {
            colWidth[col0] = std::max(colWidth[col0], ts.size());
            for (auto const &k: keys) {
                auto it = row.find(k);
                if (it != row.end()) {
                    colWidth[k] = std::max(colWidth[k], it->second.size());
                }
            }
        }

        // 3) Print header
        std::cout << std::left << std::setw(colWidth[col0]) << col0;
        for (auto const &k: keys) {
            std::cout << " | " << std::right << std::setw(colWidth[k]) << k;
        }
        std::cout << "\n";

        // 4) Separator
        std::cout << std::string(colWidth[col0], '-');
        for (auto const &k: keys) {
            std::cout << "-+-" << std::string(colWidth[k], '-');
        }
        std::cout << "\n";

        // 5) Rows
        for (auto const &[ts,row]: table) {
            std::cout << std::left << std::setw(colWidth[col0]) << ts;
            for (auto const &k: keys) {
                auto it = row.find(k);
                std::string cell = (it != row.end() ? it->second : "");
                std::cout << " | " << std::right << std::setw(colWidth[k]) << cell;
            }
            std::cout << "\n";
        }
    }
} // namespace utils
