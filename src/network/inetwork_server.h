#pragma once

#include "server_context.h"
#include <asio.hpp>

class INetworkServer {
public:
    INetworkServer(asio::io_context &io, const ServerContext &context)
        : ioContext_(io), context_(context) {
    }

    virtual void start() = 0;

    virtual ~INetworkServer() = default;

protected:
    asio::io_context &ioContext_;
    const ServerContext &context_;
};
