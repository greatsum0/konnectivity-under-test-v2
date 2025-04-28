#pragma once

#include <pcap/pcap.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <condition_variable>

/// Monitors all IP packets on `iface` to or from `port`, aggregating
/// bytes in/out and calling your callback once per interval.
class PortTrafficMonitor {
public:
    /// Called every intervalSec with (bytesIn, bytesOut)
    using Callback = std::function<void(uint64_t, uint64_t)>;

    /// @param iface        e.g. "eth0" or "lo0"
    /// @param port         TCP or UDP port to filter
    /// @param intervalSec  how often (seconds) to report metrics
    PortTrafficMonitor(std::string iface,
                       uint16_t port,
                       double intervalSec);

    ~PortTrafficMonitor();

    /// Start sniffing & timer loop.  Returns immediately.
    void start(Callback cb);

    /// Stop monitoring (blocks until threads exit).
    void stop();

private:
    void runPcap();

    void runTimer();

    static void pcapCallback(u_char *user,
                             const pcap_pkthdr *hdr,
                             const u_char *packet);

    std::string iface_;
    uint16_t port_;
    double intervalSec_;

    pcap_t *handle_ = nullptr;
    bpf_program filterProg_{};
    int linkType_ = 0;

    std::thread pcapThread_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};

    // aggregated counters
    std::atomic<uint64_t> bytesIn_{0}, bytesOut_{0};
    // our local IPv4 address on iface_, in network byte order
    uint32_t localAddr_{0};

    Callback callback_;
};
