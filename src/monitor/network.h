#pragma once
/**
 * @file network.h
 * @brief PacketSniffer – header‑only, C++20 + libpcap
 *
 *   • Captures packets on a given interface/port and exposes per‑packet metrics
 *     in the unified `Metric` format (see metric.h).
 *   • Lock‑free double buffering (single‑producer / single‑consumer).
 *   • Robust one‑way delay (OWD) reconstruction that works on Linux as well as
 *     BSD/macOS loopback by parsing the actual link & transport headers instead
 *     of hard‑coded offsets.
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

#if defined(__linux__)
#   include <endian.h>   // be64toh()
#else
#   include <arpa/inet.h>
#   define be64toh(x) ntohll(x)
#endif

#include "metric.h" // Metric, clock, key::

namespace kut::measurement::network {
    class PacketSniffer {
        /* Canonical metric identifiers (see metric.h) */
        static constexpr std::string_view kLenKey = key::net_pck_len;
        static constexpr std::string_view kOwdKey = key::net_lat_owd;

    public:
        PacketSniffer(std::string iface, uint16_t port,
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

        /* ───────────────────────── life‑cycle ────────────────────────── */
        bool start() {
            if (running_.exchange(true)) return true; // already active

            char err[PCAP_ERRBUF_SIZE]{};
            handle_ = pcap_open_live(iface_.c_str(), snaplen_, 1, timeoutMs_, err);
            if (!handle_) {
                fprintf(stderr, "[pcap] open_live: %s\n", err);
                running_ = false;
                return false;
            }
            pcap_setdirection(handle_, PCAP_D_INOUT);
            /* compile + install a BPF filter for the requested port */
            const std::string filter = "tcp port " + std::to_string(port_) +
                                       " or udp port " + std::to_string(port_);
            bpf_program prog{};
            if (pcap_compile(handle_, &prog, filter.c_str(), 1,
                             PCAP_NETMASK_UNKNOWN) != 0 ||
                pcap_setfilter(handle_, &prog) != 0) {
                fprintf(stderr, "[pcap] setfilter: %s\n", pcap_geterr(handle_));
                cleanupPcap();
                return false;
            }
            pcap_freecode(&prog);

            /* discover link‑layer header length once */
            switch (pcap_datalink(handle_)) {
                case DLT_NULL: linkHdrLen_ = 4;
                    break; // BSD loopback
                case DLT_EN10MB: linkHdrLen_ = 14;
                    break; // Ethernet
                case DLT_LINUX_SLL:
                case DLT_LINUX_SLL2: linkHdrLen_ = 16;
                    break; // Linux cooked
                default:
                    fprintf(stderr, "[pcap] unsupported DLT %d\n", pcap_datalink(handle_));
                    cleanupPcap();
                    return false;
            }

            /* remember the timestamp precision (µs or ns) */
            tsMultiplierNs_ = (pcap_get_tstamp_precision(handle_) == PCAP_TSTAMP_PRECISION_NANO)
                                  ? 1ULL
                                  : 1'000ULL; // µs → ns

            /* spawn capture loop */
            thread_ = std::thread([this] {
                pcap_loop(handle_, 0, &PacketSniffer::pcapCallback,
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

        /* ───────────────────────── consumer API ──────────────────────── */
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
        /* ──────────────────────── pcap helpers ───────────────────────── */
        static void pcapCallback(u_char *u,
                                 const pcap_pkthdr *hdr,
                                 const u_char *bytes) {
            auto *self = reinterpret_cast<PacketSniffer *>(u);
            self->storeMetric(hdr, bytes);
        }

        /* Single‑producer push (no locks) */
        void storeMetric(const pcap_pkthdr *hdr, const u_char *bytes) {
            using clk = measurement::clock; // monotonic clock alias
            auto now = clk::now(); // arrival timestamp

            /* ── frame‑length metric ─────────────────────────────────── */
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

            /* ── optional OWD metric (needs sender’s 64‑bit ns timestamp) ── */
            /* locate IP header after the link‑layer header */
            const std::size_t ipOff = linkHdrLen_;
            if (hdr->caplen <= ipOff + 1) return; // not enough for version byte

            const uint8_t ver = bytes[ipOff] >> 4;
            std::size_t ipLen = 0;
            uint8_t proto = 0;
            std::size_t l4Off = 0;

            if (ver == 4) {
                ipLen = (bytes[ipOff] & 0x0F) * 4; // IHL in 32‑bit words
                if (hdr->caplen <= ipOff + ipLen) return; // truncated
                proto = bytes[ipOff + 9];
                l4Off = ipOff + ipLen;
            } else if (ver == 6) {
                ipLen = 40; // fixed IPv6 basic header
                if (hdr->caplen <= ipOff + ipLen) return;
                proto = bytes[ipOff + 6];
                l4Off = ipOff + ipLen;
            } else {
                return; // not IPv4/6
            }

            uint64_t sendNs = 0;
            bool haveTs = false;

            if (proto == 0x11) {
                /* UDP */
                const std::size_t tsPos = l4Off + 8; // UDP header = 8 B
                if (hdr->caplen >= tsPos + sizeof(sendNs)) {
                    std::memcpy(&sendNs, bytes + tsPos, sizeof(sendNs));
                    sendNs = be64toh(sendNs);
                    haveTs = true;
                }
            } else if (proto == 0x06) {
                /* TCP */
                const uint8_t dataOff = (bytes[l4Off + 12] >> 4) & 0xF; // 32‑bit words
                const std::size_t tcpHdrLen = dataOff * 4;
                const std::size_t tsPos = l4Off + tcpHdrLen;
                if (hdr->caplen >= tsPos + sizeof(sendNs)) {
                    std::memcpy(&sendNs, bytes + tsPos, sizeof(sendNs));
                    sendNs = be64toh(sendNs);
                    haveTs = true;
                }
            }

            if (haveTs) {
                const uint64_t capNs = static_cast<uint64_t>(hdr->ts.tv_sec) * 1'000'000'000ULL +
                                       static_cast<uint64_t>(hdr->ts.tv_usec) * tsMultiplierNs_;

                uint64_t diffNs = (capNs > sendNs) ? (capNs - sendNs) : 0ULL;

                Metric d{
                    .ts = now,
                    .rel = m.rel,
                    .key = kOwdKey,
                    .value = static_cast<double>(diffNs) / 1'000'000.0
                }; // ns → ms
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

        /* ─────────────────────── data members ───────────────────────── */
        std::string iface_;
        uint16_t port_;
        int snaplen_;
        int timeoutMs_;

        pcap_t *handle_{nullptr};
        std::thread thread_;
        std::atomic<bool> running_{false};

        /* lock‑free producer/consumer buffers */
        std::vector<Metric> buffers_[2];
        std::atomic<int> activeIndex_{0};

        /* timestamp of first captured packet */
        measurement::clock::time_point firstTs_{};
        bool firstSeen_{false};

        std::size_t linkHdrLen_{0}; // length of L2 header detected at start()
        uint64_t tsMultiplierNs_{1'000}; // µs→ns (1'000) or ns (1)

        std::atomic<std::size_t> packetCount_{0};
    };
} // namespace kut::measurement::network
