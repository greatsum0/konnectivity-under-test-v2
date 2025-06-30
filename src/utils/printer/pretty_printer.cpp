// #include "pretty_printer.h"
// #include <iostream>
// #include <iomanip>
// #include <map>
// #include <set>
// #include <chrono>
// #include <ctime>
// #include <sstream>
//
// namespace utils {
//     void PrettyPrinter::printPivotTable(const std::vector<kut::measurement::Metric> &metrics) {
//         // 1) Build sets of unique timestamps (as strings) & keys
//         std::map<std::string, std::map<std::string, std::string> > table;
//         std::set<std::string> keys;
//
//         for (const auto &m: metrics) {
//             // format timestamp with milliseconds
//             const std::chrono::system_clock::time_point tp = m.ts;
//             auto ttime = std::chrono::system_clock::to_time_t(tp);
//             auto duration = tp.time_since_epoch();
//             auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
//
//             std::tm tm = *std::localtime(&ttime);
//             std::ostringstream tos;
//             tos << std::put_time(&tm, "%F %T")
//                     << "." << std::setfill('0') << std::setw(3) << millis.count();
//             std::string ts = tos.str();
//
//             keys.insert(std::set<std::string>::value_type(m.key));
//
//             std::ostringstream valoss;
//             valoss << std::fixed << std::setprecision(2) << m.value;
//             table[ts][std::set<std::string>::value_type(m.key)] = valoss.str();
//         }
//
//         // 2) Determine column widths
//         std::string col0 = "Timestamp";
//         std::map<std::string, size_t> colWidth;
//         colWidth[col0] = col0.size();
//         for (const auto &k: keys) {
//             colWidth[k] = k.size();
//         }
//
//         for (const auto &[ts, row]: table) {
//             colWidth[col0] = std::max(colWidth[col0], ts.size());
//             for (const auto &k: keys) {
//                 auto it = row.find(k);
//                 if (it != row.end()) {
//                     colWidth[k] = std::max(colWidth[k], it->second.size());
//                 }
//             }
//         }
//
//         // 3) Print header
//         std::cout << std::left << std::setw(colWidth[col0]) << col0;
//         for (const auto &k: keys) {
//             std::cout << " | " << std::right << std::setw(colWidth[k]) << k;
//         }
//         std::cout << "\n";
//
//         // 4) Separator
//         std::cout << std::string(colWidth[col0], '-');
//         for (const auto &k: keys) {
//             std::cout << "-+-" << std::string(colWidth[k], '-');
//         }
//         std::cout << "\n";
//
//         // 5) Rows
//         for (const auto &[ts, row]: table) {
//             std::cout << std::left << std::setw(colWidth[col0]) << ts;
//             for (const auto &k: keys) {
//                 auto it = row.find(k);
//                 std::string cell = (it != row.end() ? it->second : "");
//                 std::cout << " | " << std::right << std::setw(colWidth[k]) << cell;
//             }
//             std::cout << "\n";
//         }
//     }
// }
