#include "server_mode.h"
#include <iostream>
#include <utility>

ServerMode::ServerMode(ServerContext  context)
    : context(std::move(context)) {}

void ServerMode::run() const {
    std::cout << "Running server mode: " << context.name << "\n";
    std::cout << "Listening on protocol: " << context.protocol
              << " port: " << context.port << "\n";
    std::cout << "Metrics will be collected every "
              << context.metricIntervalSec << " seconds\n";
}
