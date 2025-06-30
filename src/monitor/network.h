#pragma once
/**
 * @file network.h
 * @brief Header-only packet sniffer that stores per-packet metrics in
 *        lock-free double buffers and exports them in the unified
 *        `Metric` format (see metric.h).
 *
 * Requirements
 * ────────────
 *   • C++20
 *   • libpcap
 *
 * Notes
 * ─────
 *   • Time stamps are taken from a monotonic clock (`measurement::clock`)
 *     at callback arrival; wall-clock times from libpcap are used only
 *     for one-way-delay reconstruction.
 *   • Two buffers (`buffers_[2]`) implement single-producer / single-consumer
 *     semantics with an atomic index switch, eliminating locks on
 *     the hot path.
 */

#include <pcap/pcap.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "metric.h"   // Metric, clock, key::

namespace kut::measurement::network {
    class PacketSniffer {
        /* Canonical metric identifiers (see metric.h) */
        static constexpr std::string_view kLenKey = key::net_pck_len;
        static constexpr std::string_view kOwdKey = key::net_lat_owd;

    public:
        PacketSniffer(std::string iface,
                      uint16_t port,
                      int snaplen = 65'536,
                      int timeoutMs = 10)
            : iface_(std::move(iface)),
              port_(port),
              snaplen_(snaplen),
              timeoutMs_(timeoutMs) {
            buffers_[0].reserve(4'096);
            buffers_[1].reserve(4'096);
        }

        ~PacketSniffer() { stop(); }

        // ──────────────────────────────────────────────────────────────
        // Lifecycle
        // ──────────────────────────────────────────────────────────────
        bool start() {
            if (running_.exchange(true)) return true; // already active

            char err[PCAP_ERRBUF_SIZE]{};
            handle_ = pcap_open_live(iface_.c_str(), snaplen_, 1, timeoutMs_, err);
            if (!handle_) {
                fprintf(stderr, "[pcap] open_live: %s\n", err);
                running_ = false;
                return false;
            }

            const std::string filter = "tcp port " + std::to_string(port_) +
                                       " or udp port " + std::to_string(port_);
            bpf_program prog{};
            if (pcap_compile(handle_, &prog, filter.c_str(), 1, PCAP_NETMASK_UNKNOWN) != 0 ||
                pcap_setfilter(handle_, &prog) != 0) {
                fprintf(stderr, "[pcap] setfilter: %s\n", pcap_geterr(handle_));
                cleanupPcap();
                return false;
            }
            pcap_freecode(&prog);

            thread_ = std::thread([this] {
                pcap_loop(handle_, 0,
                          &PacketSniffer::pcapCallback,
                          reinterpret_cast<u_char *>(this));
            });
            return true;
        }

        void stop() {
            if (!running_.exchange(false)) return; // inactive
            if (handle_) pcap_breakloop(handle_);
            if (thread_.joinable()) thread_.join();
            cleanupPcap();
        }

        // ──────────────────────────────────────────────────────────────
        // Consumer API
        // ──────────────────────────────────────────────────────────────
        [[nodiscard]]
        std::vector<Metric> consume() {
            const int readIdx = activeIndex_.load(std::memory_order_acquire);
            const int writeIdx = 1 - readIdx;
            activeIndex_.store(writeIdx, std::memory_order_release);

            std::vector<Metric> out;
            out.swap(buffers_[readIdx]);
            buffers_[readIdx].clear();
            return out;
        }

        [[nodiscard]]
        std::size_t peekCount() const noexcept {
            return packetCount_.load(std::memory_order_relaxed);
        }

    private:
        // ──────────────────────────────────────────────────────────────
        // pcap helpers
        // ──────────────────────────────────────────────────────────────
        static void pcapCallback(u_char *u,
                                 const pcap_pkthdr *hdr,
                                 const u_char *bytes) {
            auto *self = reinterpret_cast<PacketSniffer *>(u);
            self->storeMetric(hdr, bytes);
        }

        /* Single-producer push (no locks) */
        void storeMetric(const pcap_pkthdr *hdr, const u_char *bytes) {
            using clk = measurement::clock; // monotonic clock alias
            auto now = clk::now(); // arrival timestamp

            // ── frame-length metric ─────────────────────────────────
            Metric m{
                .ts = now,
                .rel = 0.0, // filled below
                .key = kLenKey,
                .value = static_cast<double>(hdr->len)
            };

            if (!firstSeen_) {
                firstTs_ = now;
                firstSeen_ = true;
            }
            m.rel = std::chrono::duration<double>(now - firstTs_).count();

            buffers_[activeIndex_.load(std::memory_order_relaxed)]
                    .push_back(std::move(m));

            // ── optional OWD metric (if sender timestamp present) ────
            constexpr std::size_t kNullHdr = 4; // DLT_NULL header length
            constexpr std::size_t kUdpTsOff = 0x20;
            constexpr std::size_t kTcpTsOff = 0x38;

            uint64_t sendNs = 0;
            bool haveTs = false;
            const auto proto = bytes[kNullHdr + 9]; // 0x06 = TCP, 0x11 = UDP

            if (proto == 0x11 && hdr->caplen >= kUdpTsOff + sizeof(sendNs)) {
                std::memcpy(&sendNs, bytes + kUdpTsOff, sizeof(sendNs));
                haveTs = true;
            } else if (proto == 0x06 && hdr->caplen >= kTcpTsOff + sizeof(sendNs)) {
                std::memcpy(&sendNs, bytes + kTcpTsOff, sizeof(sendNs));
                haveTs = true;
            }

            if (haveTs) {
                const uint64_t capNs =
                        static_cast<uint64_t>(hdr->ts.tv_sec) * 1'000'000'000ULL +
                        static_cast<uint64_t>(hdr->ts.tv_usec) * 1'000ULL;

                uint64_t diffNs = (capNs > sendNs) ? (capNs - sendNs) : 0ULL;
                if (diffNs > 1'000'000'000ULL) diffNs = 0ULL; // sanity clamp

                Metric d{
                    .ts = now,
                    .rel = m.rel,
                    .key = kOwdKey,
                    .value = static_cast<double>(diffNs) / 1'000'000.0 // → ms
                };
                buffers_[activeIndex_.load(std::memory_order_relaxed)]
                        .push_back(std::move(d));
            }

            packetCount_.fetch_add(1, std::memory_order_relaxed);
        }

        void cleanupPcap() {
            if (handle_) {
                pcap_close(handle_);
                handle_ = nullptr;
            }
        }

        // ──────────────────────────────────────────────────────────────
        // Data members
        // ──────────────────────────────────────────────────────────────
        std::string iface_;
        uint16_t port_;
        int snaplen_;
        int timeoutMs_;

        pcap_t *handle_{nullptr};
        std::thread thread_;
        std::atomic<bool> running_{false};

        std::vector<Metric> buffers_[2];
        std::atomic<int> activeIndex_{0};

        measurement::clock::time_point firstTs_{};
        bool firstSeen_{false};

        std::atomic<std::size_t> packetCount_{0};
    };
} // namespace kut::measurement::network
