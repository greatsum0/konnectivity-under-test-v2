// src/metrics/port_traffic_monitor.cpp
#include "port_traffic_monitor.h"

#include <pcap/pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>      // for struct tcphdr
#include <netinet/udp.h>      // for struct udphdr
#include <arpa/inet.h>
#include <ifaddrs.h>          // for getifaddrs
#include <net/if.h>
#include <cstring>
#include <iostream>

PortTrafficMonitor::PortTrafficMonitor(std::string iface,
                                       uint16_t port,
                                       double intervalSec)
    : iface_(std::move(iface)),
      port_(port),
      intervalSec_(intervalSec) {
    // determine our IPv4 address on iface_
    struct ifaddrs *addrs = nullptr;
    if (getifaddrs(&addrs) == 0) {
        for (auto *ifa = addrs; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr
                && ifa->ifa_addr->sa_family == AF_INET
                && iface_ == ifa->ifa_name) {
                auto *sin = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
                localAddr_ = sin->sin_addr.s_addr;
                break;
            }
        }
        freeifaddrs(addrs);
    }
}

PortTrafficMonitor::~PortTrafficMonitor() {
    stop();
}

void PortTrafficMonitor::start(Callback cb) {
    if (running_) return;
    callback_ = std::move(cb);
    running_ = true;

    char errbuf[PCAP_ERRBUF_SIZE];
    handle_ = pcap_open_live(iface_.c_str(), 65536, 1, 1000, errbuf);
    if (!handle_) {
        throw std::runtime_error("pcap_open_live failed: " + std::string(errbuf));
    }

    std::string expr = "port " + std::to_string(port_);
    if (pcap_compile(handle_, &filterProg_, expr.c_str(), 1, PCAP_NETMASK_UNKNOWN) < 0 ||
        pcap_setfilter(handle_, &filterProg_) < 0) {
        throw std::runtime_error("pcap_setfilter failed: " + std::string(pcap_geterr(handle_)));
    }

    linkType_ = pcap_datalink(handle_);

    pcapThread_ = std::thread(&PortTrafficMonitor::runPcap, this);
    timerThread_ = std::thread(&PortTrafficMonitor::runTimer, this);
}

void PortTrafficMonitor::stop() {
    if (!running_) return;
    running_ = false;
    if (handle_) pcap_breakloop(handle_);
    if (pcapThread_.joinable()) pcapThread_.join();
    if (timerThread_.joinable()) timerThread_.join();
    if (handle_) {
        pcap_freecode(&filterProg_);
        pcap_close(handle_);
        handle_ = nullptr;
    }
}

void PortTrafficMonitor::runPcap() {
    pcap_loop(handle_, 0, &PortTrafficMonitor::pcapCallback,
              reinterpret_cast<u_char *>(this));
}

void PortTrafficMonitor::pcapCallback(u_char *user,
                                      const struct pcap_pkthdr *hdr,
                                      const u_char *packet) {
    auto *self = reinterpret_cast<PortTrafficMonitor *>(user);

    // skip link-layer header
    const u_char *ptr = packet;
    switch (self->linkType_) {
        case DLT_EN10MB: ptr += 14;
            break; // Ethernet
        case DLT_NULL: ptr += 4;
            break; // BSD loopback
        case DLT_LOOP: ptr += 4;
            break; // alternate loopback
        case DLT_RAW: /* no header */ break;
        default: break;
    }

    // parse IPv4 header
    auto *iph = reinterpret_cast<const struct ip *>(ptr);
    int ipLenBytes = iph->ip_hl * 4;

    uint16_t srcPort = 0, dstPort = 0;
    int payloadLen = 0;

    if (iph->ip_p == IPPROTO_TCP) {
        auto *tcph = reinterpret_cast<const struct tcphdr *>(
            ptr + ipLenBytes
        );
        srcPort = ntohs(tcph->th_sport);
        dstPort = ntohs(tcph->th_dport);
        payloadLen = ntohs(iph->ip_len)
                     - ipLenBytes
                     - (tcph->th_off * 4);
    } else if (iph->ip_p == IPPROTO_UDP) {
        auto *udph = reinterpret_cast<const struct udphdr *>(
            ptr + ipLenBytes
        );
        srcPort = ntohs(udph->uh_sport);
        dstPort = ntohs(udph->uh_dport);
        payloadLen = ntohs(udph->uh_ulen) - sizeof(struct udphdr);
    } else {
        return;
    }

    // classify by our local IP rather than port direction
    if (iph->ip_src.s_addr == self->localAddr_) {
        self->bytesOut_ += payloadLen;
    } else if (iph->ip_dst.s_addr == self->localAddr_) {
        self->bytesIn_ += payloadLen;
    }
}

void PortTrafficMonitor::runTimer() {
    using namespace std::chrono;
    auto next = steady_clock::now() + duration<double>(intervalSec_);

    while (running_) {
        std::this_thread::sleep_until(next);
        next += duration<double>(intervalSec_);

        uint64_t in = bytesIn_.exchange(0);
        uint64_t out = bytesOut_.exchange(0);
        if (callback_) {
            callback_(in, out);
        }
    }
}
