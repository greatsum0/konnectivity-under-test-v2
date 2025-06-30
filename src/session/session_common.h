#pragma once
/**
 * @file common.h
 * @brief Protocol helpers, metric bucketiser, and tabular printer.
 *
 * Updated to accept the unified `Metric` structure
 * (`ts`, `rel`, `key`, `value`) introduced in metric.h.
 */

#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <limits>
#include <ostream>


#include "metric.h"

namespace kut::session::common {
    /* ------------------------------------------------------------------ */
    /* Transport protocol helpers                                         */
    /* ------------------------------------------------------------------ */
    enum class Protocol { Udp, Tcp, Unknown };

    inline Protocol toProtocol(std::string_view s) {
        if (s == "udp") return Protocol::Udp;
        if (s == "tcp") return Protocol::Tcp;
        return Protocol::Unknown;
    }

    inline std::string toProtocolString(Protocol p) {
        switch (p) {
            case Protocol::Udp: return "UDP";
            case Protocol::Tcp: return "TCP";
            default: return "Unknown";
        }
    }


    using measurement::Metric;
    using measurement::clock;

    /*────────────────────────────  helpers  ───────────────────────────────────*/
    namespace detail {
        inline bool is_cpu(std::string_view key) { return key.rfind("cpu.", 0) == 0; }
        inline bool is_net(std::string_view key) { return key.rfind("net.", 0) == 0; }
        inline double NaN() { return std::numeric_limits<double>::quiet_NaN(); }
    } // namespace detail

    /*────────────────────────────  output row  ────────────────────────────────*/
    struct Row {
        clock::time_point ts{}; // anchor time-stamp (== CPU sample)

        /* CPU metrics (may be NaN if not provided) */
        double cpu_total = detail::NaN();
        double cpu_self = detail::NaN();
        double cpu_sot = detail::NaN();
        double cpu_soc = detail::NaN();
        std::vector<double> cpu_core;

        /* derived network metrics in the interval up to next CPU sample */
        std::size_t pkts = 0; // packet count
        double net_lat_ms = detail::NaN();
        std::uint64_t net_len = 0; // total bytes
        double net_rate = detail::NaN(); /* Mbps */
    };

    /*────────────────────────────  core logic  ────────────────────────────────*/
    inline std::vector<Row> compute(std::vector<Metric> data) {
        /* 0. make sure data are sorted */
        std::sort(data.begin(), data.end(),
                  [](const Metric &a, const Metric &b) { return a.ts < b.ts; });

        std::vector<Row> out;
        Row cur;
        bool have_anchor = false;

        /* rolling accumulators for current CPU-to-CPU window */
        std::size_t lat_ct = 0;
        double lat_sum = 0.0;
        clock::time_point first_net{}, last_net{};
        bool have_net = false;

        const auto flush = [&] {
            if (!have_anchor) return;
            if (cur.pkts) {
                const double dur_s =
                        std::chrono::duration<double>(last_net - first_net).count();
                if (dur_s > 0.0)
                    cur.net_rate = 8.0 * cur.net_len / dur_s / 1'000'000.0;
                cur.net_lat_ms = lat_ct
                                     ? lat_sum / static_cast<double>(lat_ct)
                                     : detail::NaN();
            }
            out.emplace_back(cur);

            /* reset network state */
            cur.pkts = 0;
            cur.net_len = 0;
            cur.net_lat_ms = detail::NaN();
            cur.net_rate = detail::NaN();
            lat_ct = 0;
            lat_sum = 0.0;
            have_net = false;
        };

        for (const auto &m: data) {
            /* 1. CPU anchor rows (may arrive as several metrics sharing the same ts) */
            if (detail::is_cpu(m.key)) {
                if (!have_anchor || m.ts != cur.ts) {
                    flush();
                    cur = Row{};
                    cur.ts = m.ts;
                    have_anchor = true;
                }
                if (m.key == measurement::key::cpu_total) cur.cpu_total = m.value;
                else if (m.key == measurement::key::cpu_self) cur.cpu_self = m.value;
                else if (m.key == measurement::key::cpu_sot) cur.cpu_sot = m.value;
                else if (m.key == measurement::key::cpu_soc) cur.cpu_soc = m.value;
                else if (m.key.rfind("cpu.core.", 0) == 0) {
                    const std::size_t idx = std::stoul(std::string(m.key.substr(9)));
                    if (idx >= cur.cpu_core.size()) cur.cpu_core.resize(idx + 1, detail::NaN());
                    cur.cpu_core[idx] = m.value;
                }
                continue;
            }

            /* 2. network metrics between CPU samples */
            if (detail::is_net(m.key) && have_anchor) {
                if (!have_net) {
                    first_net = m.ts;
                    have_net = true;
                }
                last_net = m.ts;

                if (m.key == measurement::key::net_pck_len) {
                    ++cur.pkts;
                    cur.net_len += static_cast<std::uint64_t>(m.value);
                } else if (m.key == measurement::key::net_lat_owd) {
                    lat_sum += m.value;
                    ++lat_ct;
                }
            }
        }
        flush(); // final slice
        return out;
    }

    /*────────────────────────────  CSV helper  ────────────────────────────────*/
    inline void write_csv(const std::vector<Row> &rows, std::ostream &os) {
        /* ─ determine max #cores seen ─ */
        std::size_t maxC = 0;
        for (auto &r: rows) maxC = std::max(maxC, r.cpu_core.size());

        /* ─ header ─ */
        os << "ts,cpu.total,cpu.self,cpu.sot,cpu.soc";
        for (std::size_t i = 0; i < maxC; ++i) os << ",cpu.core." << i;
        os << ",pkts,net.lat.owd.ms,net.pck.len,net.rate.mbps\n";

        os << std::fixed << std::setprecision(6);

        /* ─ rows ─ */
        for (const auto &r: rows) {
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                r.ts.time_since_epoch()).count();
            os << ns << ','
                    << r.cpu_total << ','
                    << r.cpu_self << ','
                    << r.cpu_sot << ','
                    << r.cpu_soc;
            for (std::size_t i = 0; i < maxC; ++i)
                os << ',' << (i < r.cpu_core.size() ? r.cpu_core[i] : detail::NaN());
            os << ',' << r.pkts
                    << ',' << r.net_lat_ms
                    << ',' << r.net_len
                    << ',' << r.net_rate << '\n';
        }
    }
} // namespace kut::session::common
