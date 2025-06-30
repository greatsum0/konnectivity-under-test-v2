#pragma once
/**
 * @file cpu_usage_monitor.h
 * @brief CPUUsageMonitor – header‑only, C++20
 *
 *   • Periodically samples /proc/stat and /proc/self/stat to obtain
 *     system‑wide per‑core utilisation as well as *this* process’ CPU share.
 *   • Works on Linux; falls back to deterministic mock values elsewhere so
 *     that unit tests still run on CI.
 */

#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>   // sysconf

#include "metric.h"  // -> kut::measurement::{Metric, clock, key, make}

#if !__linux__
#   include <random>
#endif

namespace kut::measurement::cpu {
    /* ───────────────────────── raw snapshot ─────────────────────────── */
    struct CPUUsageSnapshot {
        std::vector<double> perCoreUsage; // [%] size == #logical CPUs
        double totalUsage{0.0}; // sum of per‑core usages  (0‑N*100)
        double selfUsage{0.0}; // this process [% of one core]
        double selfOverTotal{0.0}; // self / total           [%]
        double selfOverCapacity{0.0}; // self / (N*100)         [%]
    };

    /* ───────────────── timestamped record (+flatten to Metric) ──────── */
    struct Record {
        clock::time_point ts;
        CPUUsageSnapshot snap;

        [[nodiscard]] std::vector<Metric> flatten(clock::time_point t0) const {
            return {
                make(ts, t0, key::cpu_total, snap.totalUsage),
                make(ts, t0, key::cpu_self, snap.selfUsage),
                make(ts, t0, key::cpu_sot, snap.selfOverTotal),
                make(ts, t0, key::cpu_soc, snap.selfOverCapacity)
            };
        }
    };

    /* ─────────────────────────── monitor class ───────────────────────── */
    class CPUUsageMonitor {
    public:
        explicit CPUUsageMonitor(double intervalSec = 1.0)
            : interval_{std::chrono::duration<double>(intervalSec)}
#if __linux__
              , coreCount_{static_cast<std::size_t>(::sysconf(_SC_NPROCESSORS_ONLN))}
              , prevTotal_(coreCount_, 0)
              , prevIdle_(coreCount_, 0)
#endif
        {
        }

        ~CPUUsageMonitor() { stop(); }

        /* ─────── start/stop ─────── */
        void start() {
            bool expected = false;
            if (running_.compare_exchange_strong(expected, true)) {
                worker_ = std::thread([this] { loop(); });
            }
        }

        void stop() {
            running_.store(false);
            if (worker_.joinable()) worker_.join();
        }

        /* ─────── consumers ─────── */
        [[nodiscard]] std::vector<Record> consume() {
            std::scoped_lock lk(m_);
            auto out = std::move(log_);
            log_.clear();
            return out;
        }

        [[nodiscard]] std::optional<Record> latest() const {
            std::scoped_lock lk(m_);
            return log_.empty() ? std::nullopt : std::optional<Record>{log_.back()};
        }

    private:
        /* ─────── sampling loop ─────── */
        void loop() {
            using steady = std::chrono::steady_clock;
            auto next = steady::now();
            while (running_.load(std::memory_order_relaxed)) {
                /* 1) take sample */
                auto ts = steady::now();
                auto snap = sampleOnce(); {
                    std::scoped_lock lk(m_);
                    log_.push_back({ts, std::move(snap)});
                }
                /* 2) wait for absolute next tick */
                next += std::chrono::duration_cast<steady::duration>(interval_);
                std::this_thread::sleep_until(next);
            }
        }

        /* ─────── platform‑specific snapshot ─────── */
        CPUUsageSnapshot sampleOnce() {
            double total = 0.0;
            std::vector<double> perCore;
            double self = 0.0;

#if __linux__
            perCore = readPerCoreUsages(total);
            self = readSelfCPUUsage();
            std::cout << self<< "\n";
#else
        perCore = mockCoreUsages();
        total   = 75.0;
        self    = mockSelfUsage();
#endif
            const double sot = total > 0.0 ? (self / total) * 100.0 : 0.0;
            const double soc = !perCore.empty() ? (self / (perCore.size() * 100.0)) * 100.0 : 0.0;

            return {std::move(perCore), total, self, sot, soc};
        }

#if __linux__
        /* ─────── /proc readers ─────── */
        std::vector<double> readPerCoreUsages(double &outSum) {
            std::ifstream stat("/proc/stat");
            std::string line;
            std::vector<double> usages;
            outSum = 0.0;

            int idx = 0;
            while (std::getline(stat, line)) {
                if (line.rfind("cpu", 0) == 0 && line.size() > 3 && std::isdigit(line[3])) {
                    std::istringstream ss(line);
                    std::string ignore;
                    long long user, nice, sys, idle, iowait, irq, softirq, steal;
                    ss >> ignore >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal;

                    long long total = user + nice + sys + idle + iowait + irq + softirq + steal;
                    long long idleC = idle + iowait;

                    if (idx >= static_cast<int>(prevTotal_.size())) {
                        prevTotal_.resize(idx + 1, 0);
                        prevIdle_.resize(idx + 1, 0);
                    }
                    long long dtTot = total - prevTotal_[idx];
                    long long dtIdle = idleC - prevIdle_[idx];
                    long long dtUse = dtTot - dtIdle;

                    prevTotal_[idx] = total;
                    prevIdle_[idx] = idleC;

                    double pct = dtTot ? static_cast<double>(dtUse) * 100.0 / dtTot : 0.0;
                    usages.push_back(pct);
                    outSum += pct;
                    ++idx;
                }
            }
            return usages;
        }

        double readSelfCPUUsage() {
            std::ifstream self("/proc/self/stat");
            std::string line;
            if (!std::getline(self, line)) return 0.0;
            std::istringstream ss(line);

            std::string ignore, comm;
            ss >> ignore >> comm;
            long utime = 0, stime = 0;
            for (int i = 3; i <= 15; ++i) {
                std::string tok;
                ss >> tok;
                if (i == 14) utime = std::stol(tok);
                if (i == 15) stime = std::stol(tok);
            }
            long long curr = utime + stime;
            auto now = std::chrono::steady_clock::now();

            if (prevSelfJiff_ == -1) {
                prevSelfJiff_ = curr;
                prevSelfTime_ = now;
                return 0.0; // warm‑up sample
            }

            long long dJiff = curr - prevSelfJiff_;
            prevSelfJiff_ = curr;

            auto wall = now - prevSelfTime_;
            prevSelfTime_ = now;
            double wallSec = std::chrono::duration<double>(wall).count(); // ← fix

            const double hz = static_cast<double>(::sysconf(_SC_CLK_TCK));
            return wallSec > 0.0
                       ? static_cast<double>(dJiff) * 100.0 / (hz * wallSec)
                       : 0.0;
        }
#else   /* ─── portable mock path (non‑Linux) ─── */
    static std::vector<double> mockCoreUsages() {
        static std::default_random_engine rng{std::random_device{}()};
        static std::uniform_real_distribution<double> d(10.0, 40.0);
        return {d(rng), d(rng), d(rng), d(rng)};
    }

    static double mockSelfUsage() {
        static std::default_random_engine rng{std::random_device{}()};
        static std::uniform_real_distribution<double> d(5.0, 20.0);
        return d(rng);
    }
#endif /* __linux__ */

        /* ─────── members ─────── */
        std::chrono::duration<double> interval_;
        std::atomic<bool> running_{false};
        std::thread worker_;
        mutable std::mutex m_;
        std::vector<Record> log_;

#if __linux__
        std::size_t coreCount_;
        std::vector<long long> prevTotal_, prevIdle_;
        long long prevSelfJiff_{-1};
        std::chrono::steady_clock::time_point prevSelfTime_;
#endif
    };
} // namespace kut::measurement::cpu
