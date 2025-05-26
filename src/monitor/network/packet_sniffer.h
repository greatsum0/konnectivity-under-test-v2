#pragma once

#include "packet_record.h"
#include <pcap.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>

class PacketSniffer {
public:
    using PacketHandler = std::function<void(const PacketRecord&)>;

    PacketSniffer(std::string iface, uint16_t port);
    ~PacketSniffer();

    void start(PacketHandler handler);
    void stop();

private:
    void runPcap();
    static void pcapCallback(u_char *user, const pcap_pkthdr *hdr, const u_char *packet);

    std::string iface_;
    uint16_t port_;
    uint32_t localAddr_ = 0;
    int linkType_ = 0;

    pcap_t *handle_ = nullptr;
    bpf_program filterProg_{};

    std::thread snifferThread_;
    std::atomic<bool> running_{false};
    PacketHandler packetHandler_;
};
