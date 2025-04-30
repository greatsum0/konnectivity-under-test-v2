// utils/pivot_csv_writer.cpp
#include "utils/csv_writer.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace utils {
    PivotCSVWriter::PivotCSVWriter(std::string path)
        : _path(std::move(path)) {
    }

    void PivotCSVWriter::write(const std::vector<Metric> &metrics) const {
        // 1) Pivot data into a map: timestamp → (key → value)
        std::map<std::string, std::map<std::string, std::string> > table;
        std::set<std::string> keys;

        for (auto const &m: metrics) {
            // format timestamp as "YYYY-MM-DD HH:MM:SS"
            auto tt = std::chrono::system_clock::to_time_t(m.timestamp);
            std::tm tm = *std::localtime(&tt);
            std::ostringstream tos;
            tos << std::put_time(&tm, "%F %T");
            std::string ts = tos.str();

            keys.insert(m.key);
            table[ts][m.key] = std::to_string(m.value);
        }

        // 2) Open file
        std::ofstream ofs(_path);
        if (!ofs) {
            throw std::runtime_error("Unable to open CSV file: " + _path);
        }

        // 3) Write header
        ofs << "Timestamp";
        for (auto const &k: keys) {
            ofs << ',' << k;
        }
        ofs << '\n';

        // 4) Write each row
        for (auto const &[ts, row]: table) {
            ofs << ts;
            for (auto const &k: keys) {
                ofs << ','
                        << (row.count(k) ? row.at(k) : "");
            }
            ofs << '\n';
        }
    }
} // namespace utils
