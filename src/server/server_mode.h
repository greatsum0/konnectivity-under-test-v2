#pragma once

#include "server_context.h"

class ServerMode {
public:
    explicit ServerMode(ServerContext  context);
    void run() const;

private:
    ServerContext context;
};
