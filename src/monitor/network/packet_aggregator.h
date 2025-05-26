#pragma once

#include "packet_record.h"
#include <mutex>
#include <vector>
#include <chrono>
#include "metric_types.h"

class PacketAggregator {
public:
    explicit PacketAggregator(double intervalSec);

    void addPacket(const PacketRecord &packet);

    AggregateSample sample();

private:
    double intervalSec_;
    std::mutex mutex_;
    std::vector<PacketRecord> packetLog_;
};
