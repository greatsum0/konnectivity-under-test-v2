#pragma once

class INetworkClient {
public:
    virtual void sendMessage(const std::string &message) = 0;
    virtual ~INetworkClient() = default;

    virtual void printSummary() const = 0;
};
