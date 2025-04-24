#include "client_mode.h"
#include <iostream>
#include <utility>
#include <asio/io_context.hpp>
#include "tcp_client.h"
#include "udp_client.h"

ClientMode::ClientMode(ClientContext context)
    : context(std::move(context)) {
}

void ClientMode::run() const {
    asio::io_context io;

    if (context.network.protocol == "tcp") {
        TcpClient client(io, context);
        client.connect();

        std::vector<uint8_t> payload{'h', 'e', 'l', 'l', 'o', ' ', 's', 'e', 'r', 'v', 'e', 'r'};
        client.send(payload);

        client.disconnect();
    }if (context.network.protocol == "udp") {
        UdpClient client(io, context);
        client.connect();

        std::string msg = "hello via udp";
        std::vector<uint8_t> payload(msg.begin(), msg.end());
        client.send(payload);

        client.disconnect();
    } else {
        std::cerr << "Unsupported protocol: " << context.network.protocol << "\n";
    }
}
