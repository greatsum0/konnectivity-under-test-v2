#pragma once

#include "client_context.h"

class ClientMode {
public:
    explicit ClientMode(ClientContext  context);
    void run() const;

private:
    ClientContext context;
};
