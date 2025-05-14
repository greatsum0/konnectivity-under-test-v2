#include "port_traffic_monitor.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <cstring>
#include <iostream>
#include <chrono>

namespace {
    constexpr int kEthernetHeaderLen = 14;
    constexpr int kLoopbackHeaderLen = 4;
}

PortTrafficMonitor::PortTrafficMonitor(std::string iface, const uint16_t port, double intervalSec)
    : iface_(std::move(iface)), port_(port), intervalSec_(intervalSec) {
    struct ifaddrs *addrs = nullptr;
    if (getifaddrs(&addrs) == 0) {
        for (const auto *ifa = addrs; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface_ == ifa->ifa_name) {
                const auto *sin = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
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

PortTrafficMonitor::~PortTrafficMonitor() {
    stop();
}

void PortTrafficMonitor::start(Callback cb) {
    if (running_) return;
    aggregateCallback_ = std::move(cb);
    running_ = true;

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
    pcap_loop(handle_, 0, &PortTrafficMonitor::pcapCallback, reinterpret_cast<u_char *>(this));
}

void PortTrafficMonitor::pcapCallback(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet) {
    auto *self = reinterpret_cast<PortTrafficMonitor *>(user);
    const u_char *ptr = packet;

    switch (self->linkType_) {
        case DLT_EN10MB: ptr += 14;
            break;
        case DLT_NULL:
        case DLT_LOOP: ptr += 4;
            break;
        default:
            return;
    }

    const auto iph = reinterpret_cast<const struct ip *>(ptr);
    int ipLenBytes = iph->ip_hl * 4;
    int payloadLen = 0;

    if (iph->ip_p == IPPROTO_TCP) {
        const auto *tcph = reinterpret_cast<const struct tcphdr *>(ptr + ipLenBytes);
        payloadLen = ntohs(iph->ip_len) - ipLenBytes - (tcph->th_off * 4);
    } else if (iph->ip_p == IPPROTO_UDP) {
        const auto *udph = reinterpret_cast<const struct udphdr *>(ptr + ipLenBytes);
        payloadLen = ntohs(udph->uh_ulen) - sizeof(struct udphdr);
    } else {
        return;
    }

    if (payloadLen <= 0) return;

    bool isOut = (iph->ip_src.s_addr == self->localAddr_);

    auto ts = std::chrono::system_clock::time_point{
        std::chrono::seconds(hdr->ts.tv_sec) +
        std::chrono::microseconds(hdr->ts.tv_usec)
    };

    std::lock_guard<std::mutex> lock(self->logMutex_);
    self->packetLog_.emplace_back(PacketRecord{ts, static_cast<size_t>(payloadLen), isOut});
}


void PortTrafficMonitor::runTimer() {
    using namespace std::chrono;
    auto next = steady_clock::now() + duration<double>(intervalSec_);

    while (running_) {
        std::this_thread::sleep_until(next);
        next += duration<double>(intervalSec_);

        std::vector<PacketRecord> snapshot; {
            std::lock_guard<std::mutex> lock(logMutex_);
            snapshot.swap(packetLog_);
        }

        AggregateSample agg{};
        agg.timestamp = system_clock::now();

        for (const auto &pkt: snapshot) {
            if (pkt.isOut) {
                agg.bytesOut += pkt.size;
                agg.packetsOut++;
            } else {
                agg.bytesIn += pkt.size;
                agg.packetsIn++;
            }
        }

        if (aggregateCallback_) {
            aggregateCallback_(agg);
        }
    }
}
