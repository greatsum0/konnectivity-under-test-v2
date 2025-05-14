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

        for (const auto &m: metrics) {
            // format timestamp with milliseconds
            auto tp = m.timestamp;
            auto ttime = std::chrono::system_clock::to_time_t(tp);
            auto duration = tp.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

            std::tm tm = *std::localtime(&ttime);
            std::ostringstream tos;
            tos << std::put_time(&tm, "%F %T")
                    << "." << std::setfill('0') << std::setw(3) << millis.count();
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
        for (const auto &k: keys) {
            ofs << ',' << k;
        }
        ofs << '\n';

        // 4) Write each row
        for (const auto &[ts, row]: table) {
            ofs << ts;
            for (const auto &k: keys) {
                ofs << ',' << (row.count(k) ? row.at(k) : "");
            }
            ofs << '\n';
        }
    }
} // namespace utils
