#include "packet_sniffer.h"
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <cstring>
#include <iostream>
#include <chrono>

namespace {
    constexpr int kEthernetHeaderLen = 14;
    constexpr int kLoopbackHeaderLen = 4;
}

PacketSniffer::PacketSniffer(std::string iface, uint16_t port)
    : iface_(std::move(iface)), port_(port) {
    struct ifaddrs *addrs = nullptr;
    if (getifaddrs(&addrs) == 0) {
        for (auto *ifa = addrs; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface_ == ifa->ifa_name) {
                const auto *sin = reinterpret_cast<sockaddr_in *>(ifa->ifa_addr);
                localAddr_ = sin->sin_addr.s_addr;
                break;
            }
        }
        freeifaddrs(addrs);
    }

    if (localAddr_ == 0) {
        std::cerr << "[WARN] Could not resolve local IP for interface: " << iface_ << "\n";
    }
}

PacketSniffer::~PacketSniffer() {
    stop();
}

void PacketSniffer::start(PacketHandler handler) {
    if (running_) return;

    running_ = true;
    packetHandler_ = std::move(handler);

    char errbuf[PCAP_ERRBUF_SIZE];
    handle_ = pcap_open_live(iface_.c_str(), BUFSIZ, 0, 1, errbuf);
    if (!handle_) {
        throw std::runtime_error("pcap_open_live failed: " + std::string(errbuf));
    }

    const std::string expr = "port " + std::to_string(port_);
    if (pcap_compile(handle_, &filterProg_, expr.c_str(), 1, PCAP_NETMASK_UNKNOWN) < 0 ||
        pcap_setfilter(handle_, &filterProg_) < 0) {
        throw std::runtime_error("pcap_setfilter failed: " + std::string(pcap_geterr(handle_)));
    }

    linkType_ = pcap_datalink(handle_);
    snifferThread_ = std::thread(&PacketSniffer::runPcap, this);
}

void PacketSniffer::stop() {
    if (!running_) return;
    running_ = false;

    if (handle_) pcap_breakloop(handle_);
    if (snifferThread_.joinable()) snifferThread_.join();

    if (handle_) {
        pcap_freecode(&filterProg_);
        pcap_close(handle_);
        handle_ = nullptr;
    }
}

void PacketSniffer::runPcap() {
    pcap_loop(handle_, 0, &PacketSniffer::pcapCallback, reinterpret_cast<u_char *>(this));
}

void PacketSniffer::pcapCallback(u_char *user, const pcap_pkthdr *hdr, const u_char *packet) {
    auto *self = reinterpret_cast<PacketSniffer *>(user);
    const u_char *ptr = packet;

    switch (self->linkType_) {
        case DLT_EN10MB: ptr += kEthernetHeaderLen;
            break;
        case DLT_NULL:
        case DLT_LOOP: ptr += kLoopbackHeaderLen;
            break;
        default: return;
    }

    const auto *iph = reinterpret_cast<const ip *>(ptr);
    const int ipLenBytes = iph->ip_hl * 4;

    int payloadLen = 0;
    if (iph->ip_p == IPPROTO_TCP) {
        const auto *tcph = reinterpret_cast<const tcphdr *>(ptr + ipLenBytes);
        payloadLen = ntohs(iph->ip_len) - ipLenBytes - (tcph->th_off * 4);
    } else if (iph->ip_p == IPPROTO_UDP) {
        const auto *udph = reinterpret_cast<const udphdr *>(ptr + ipLenBytes);
        payloadLen = ntohs(udph->uh_ulen) - sizeof(udphdr);
    } else {
        return;
    }

    if (payloadLen <= 0) return;

    const bool isOut = (iph->ip_src.s_addr == self->localAddr_);
    const auto ts = std::chrono::system_clock::time_point{
        std::chrono::seconds(hdr->ts.tv_sec) + std::chrono::microseconds(hdr->ts.tv_usec)
    };

    PacketRecord rec{ts, static_cast<size_t>(payloadLen), isOut};
    if (self->packetHandler_) {
        self->packetHandler_(rec);
    }
}
