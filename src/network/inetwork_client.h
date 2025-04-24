#pragma once
#include <vector>
#include <cstdint>

class INetworkClient {
public:
    virtual void connect() = 0;
    virtual void send(const std::vector<uint8_t>& data) = 0;
    virtual void disconnect() = 0;
    virtual ~INetworkClient() = default;
};
