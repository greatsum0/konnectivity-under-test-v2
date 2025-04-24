#include "client_mode.h"
#include <iostream>
#include <utility>

ClientMode::ClientMode(ClientContext  context)
    : context(std::move(context)) {}

void ClientMode::run() const {
    // Placeholder — daha sonra: client oluştur, traffic başlat, metrik topla
    std::cout << "Running client mode for scenario: " << context.name << "\n";
    std::cout << "Protocol: " << context.network.protocol << "\n";
    std::cout << "Server: " << context.network.serverAddress
              << ":" << context.network.serverPort << "\n";
    std::cout << "Traffic type: " << context.traffic.type << "\n";
}
