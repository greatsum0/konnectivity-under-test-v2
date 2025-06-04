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

    [[nodiscard]] AggregateSample sample() const;

private:
    std::string iface_;
    uint16_t port_;
    double intervalSec_;
    bool running_ = false;

    std::unique_ptr<PacketSniffer> sniffer_;
    std::unique_ptr<PacketAggregator> aggregator_;
};
