#pragma once
#include <chrono>
#include <string_view>
#include <string>

namespace kut::measurement {
    /**
     * @brief Unified time base for all measurements.
     *
     * Steady clocks are unaffected by NTP adjustments or system time changes,
     * ensuring monotonic and consistent interval computation.
     */
    using clock = std::chrono::steady_clock;

    /**
     * @struct Metric
     *
     * @brief A lightweight representation of a scalar measurement sample
     *        collected at a specific time point during runtime.
     *
     * This format is intended for efficient post-processing and aggregation
     * of performance data, and applies uniformly to networking and CPU metrics.
     *
     * Fields:
     *   - ts   : absolute timestamp of the measurement (monotonic)
     *   - rel  : time since experiment start (in seconds)
     *   - key  : symbolic name of the metric type (hierarchical dot notation)
     *   - value: scalar value (unit depends on metric type, always stored as double)
     */
    struct Metric {
        clock::time_point ts;
        double rel; // time since t₀ in seconds
        std::string_view key;
        double value;
    };

    /**
     * @namespace key
     *
     * @brief Symbolic constant names for all recognized metric types.
     *
     * These are intended for internal consistency and to avoid typographical
     * errors when populating or interpreting measurement data.
     */
    namespace key {
        // ────── Networking Metrics ──────

        /// Length of each captured packet [bytes]
        inline constexpr std::string_view net_pck_len = "net.pck.len";

        /// One-way delay estimate per packet [milliseconds]
        inline constexpr std::string_view net_lat_owd = "net.lat.owd.ms";

        /// Packet count per interval (for aggregated reporting)
        inline constexpr std::string_view net_pkts = "net.pkts";

        // ────── CPU Metrics ──────

        /// Total system CPU usage [%]
        inline constexpr std::string_view cpu_total = "cpu.total";

        /// CPU usage attributed to current process [%]
        inline constexpr std::string_view cpu_self = "cpu.self";

        /// Ratio of self to total CPU usage [%]
        inline constexpr std::string_view cpu_sot = "cpu.sot"; // self over total

        /// Ratio of self to system capacity [%]
        inline constexpr std::string_view cpu_soc = "cpu.soc"; // self over capacity
    }

    /**
     * @brief Convenience helper to construct a Metric record.
     *
     * @param ts   Absolute timestamp
     * @param t0   Baseline start timestamp (as steady_clock::time_point)
     * @param key  Symbolic key identifying the metric
     * @param val  Measured value
     *
     * @return Metric Fully populated Metric struct
     */
    inline Metric make(const clock::time_point ts,
                       const clock::time_point t0,
                       const std::string_view key,
                       const double val) {
        return {
            ts,
            std::chrono::duration<double>(ts - t0).count(),
            key,
            val
        };
    }
} // namespace kut::measurement
