#include "port_traffic_monitor.h"

PortTrafficMonitor::PortTrafficMonitor(std::string iface, uint16_t port, double intervalSec)
    : iface_(std::move(iface)), port_(port), intervalSec_(intervalSec) {
}

PortTrafficMonitor::~PortTrafficMonitor() {
    stop();
}

void PortTrafficMonitor::start(Callback cb) {
    if (running_) return;
    running_ = true;

    aggregator_ = std::make_unique<PacketAggregator>(intervalSec_);

    sniffer_ = std::make_unique<PacketSniffer>(iface_, port_);

    sniffer_->start([this](const PacketRecord &rec) {
        aggregator_->addPacket(rec);
    });
}

void PortTrafficMonitor::stop() {
    if (!running_) return;
    running_ = false;

    if (sniffer_) sniffer_->stop();
}

AggregateSample PortTrafficMonitor::sample() const {
    if (!aggregator_) return {};
    return aggregator_->sample();
}
