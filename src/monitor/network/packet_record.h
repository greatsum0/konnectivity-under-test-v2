#pragma once
#include <chrono>

struct PacketRecord {
    int64_t timestamp;
    int64_t embeddedTimestamp;
    size_t size{};
    bool isOut{};
};
