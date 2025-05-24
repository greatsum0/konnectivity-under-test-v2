// inetwork_server.h
#pragma once

#include <string>
#include <functional>
#include <memory>

/// Interface for an asynchronous network server (TCP or UDP)
class INetworkServer {
public:
    using ReceiveHandler = std::function<void(const std::string &)>;

    virtual ~INetworkServer() = default;

    /// Start the server: bind/listen or bind for datagrams
    virtual void start() = 0;

    /// Send a message to the connected client (TCP) or last sender (UDP)
    virtual void send(const std::string &message) = 0;

    /// Stop the server and close any open sockets
    virtual void stop() = 0;

    /// Set a callback to be invoked when a message is received
    virtual void setReceiveHandler(ReceiveHandler handler) = 0;
};
