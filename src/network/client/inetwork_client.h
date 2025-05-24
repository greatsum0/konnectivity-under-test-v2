// inetwork_client.h
#pragma once

#include <string>
#include <functional>
#include <memory>

/// Interface for an asynchronous network client (TCP or UDP)
class INetworkClient {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    virtual ~INetworkClient() = default;

    /// Start the client: resolve/connect or bind as needed
    virtual void start() = 0;

    /// Send a message to the server
    virtual void send(const std::string &message) = 0;

    /// Stop the client and close any open sockets
    virtual void stop() = 0;

    /// Set a callback to be invoked when a message is received
    virtual void setReceiveHandler(ReceiveHandler handler) = 0;
};
