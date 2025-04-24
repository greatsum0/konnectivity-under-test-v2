#include "server_mode.h"
#include <iostream>
#include <utility>
#include <asio.hpp>
#include "udp_server.h"

#include "tcp_server.h"

ServerMode::ServerMode(ServerContext context)
    : context(std::move(context)) {
}


void ServerMode::run() const {
    std::cout << "Running server mode: " << context.name << "\n";
    asio::io_context io;

    if (context.protocol == "tcp") {
        TcpServer server(io, context);
        server.start();
        io.run();
    } else if (context.protocol == "udp") {
        UdpServer server(io, context);
        server.start();
        io.run();
    } else {
        std::cerr << "Unsupported protocol: " << context.protocol << "\n";
    }
}
