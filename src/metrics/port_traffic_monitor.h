#pragma once

#include <pcap/pcap.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <vector>

class PortTrafficMonitor {
public:
    struct PacketRecord {
        std::chrono::system_clock::time_point timestamp;
        size_t size;
        bool isOut;
    };

    struct AggregateSample {
        std::chrono::system_clock::time_point timestamp;
        uint64_t bytesIn = 0;
        uint64_t bytesOut = 0;
        uint64_t packetsIn = 0;
        uint64_t packetsOut = 0;
    };

    using Callback = std::function<void(const AggregateSample&)>;

    PortTrafficMonitor(std::string iface, uint16_t port, double intervalSec);
    ~PortTrafficMonitor();

    void start(Callback cb);
    void stop();

private:
    void runPcap();
    void runTimer();
    static void pcapCallback(u_char *user, const pcap_pkthdr *hdr, const u_char *packet);

    std::string iface_;
    uint16_t port_;
    double intervalSec_;

    pcap_t *handle_ = nullptr;
    bpf_program filterProg_{};
    int linkType_ = 0;

    std::thread pcapThread_;
    std::thread timerThread_;
    std::atomic<bool> running_{false};
    uint32_t localAddr_{0};

    Callback aggregateCallback_;

    std::mutex logMutex_;
    std::vector<PacketRecord> packetLog_;
};
