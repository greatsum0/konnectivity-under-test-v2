#pragma once
#include <chrono>

struct PacketRecord {
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point embeddedTimestamp;
    size_t size{};
    bool isOut{};
};
