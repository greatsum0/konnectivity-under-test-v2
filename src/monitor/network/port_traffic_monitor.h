// #pragma once
//
// #include <pcap/pcap.h>
//
// #include <atomic>
// #include <chrono>
// #include <cstdint>
// #include <functional>
// #include <mutex>
// #include <string>
// #include <thread>
// #include <vector>
//
// class PortTrafficMonitor {
// public:
//     struct AggregateSample {
//         std::chrono::system_clock::time_point timestamp;
//         size_t bytesIn = 0;
//         size_t bytesOut = 0;
//         size_t packetsIn = 0;
//         size_t packetsOut = 0;
//         double throughputMbps = 0.0;
//     };
//
//     PortTrafficMonitor(std::string iface, uint16_t port, double intervalSec);
//     ~PortTrafficMonitor();
//
//     using Callback = std::function<void(const AggregateSample &)>;
//     void start(Callback cb);   // Callback şu an kullanılmıyor, ama uyumlu kalıyor
//     void stop();
//     AggregateSample sample();  // Dışarıdan tetiklenen örnekleme
//
// private:
//     struct PacketRecord {
//         std::chrono::system_clock::time_point timestamp;
//         size_t size;
//         bool isOut;
//     };
//
//     void runPcap();
//     static void pcapCallback(u_char *user, const pcap_pkthdr *hdr, const u_char *packet);
//
//     std::string iface_;
//     uint16_t port_;
//     double intervalSec_;
//     uint32_t localAddr_ = 0;
//
//     pcap_t *handle_ = nullptr;
//     bpf_program filterProg_{};
//     int linkType_ = 0;
//
//     std::vector<PacketRecord> packetLog_;
//     std::mutex logMutex_;
//
//     std::thread pcapThread_;
//     bool running_ = false;
// };
//

#pragma once

#include "packet_sniffer.h"
#include "packet_aggregator.h"
#include "metric_types.h"

class PortTrafficMonitor {
public:
    using Callback = std::function<void(const AggregateSample &)>;

    PortTrafficMonitor(std::string iface, uint16_t port, double intervalSec);
    ~PortTrafficMonitor();

    void start(Callback cb = nullptr);
    void stop();

    AggregateSample sample();  // dış tetikleme ile snapshot + throughput

private:
    std::string iface_;
    uint16_t port_;
    double intervalSec_;
    bool running_ = false;

    std::unique_ptr<PacketSniffer> sniffer_;
    std::unique_ptr<PacketAggregator> aggregator_;
};
