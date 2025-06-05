#pragma once
#include <chrono>

struct PacketRecord {
    std::chrono::system_clock::time_point timestamp;
    size_t size{};
    bool isOut{};
};
