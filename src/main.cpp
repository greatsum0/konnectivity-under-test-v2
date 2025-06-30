#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <client_session.h>
#include <arg_parser.h>
#include <server_session.h>
#include <yaml_parser.h>
#include "context.h"

#include "utils/logger.h"

enum class Mode {
    Server,
    Client
};

static Mode parseMode(const std::string &s) {
    std::string lower{s};
    std::ranges::transform(lower,
                           lower.begin(),
                           [](const unsigned char c) { return static_cast<char>(std::tolower(c)); }
    );

    if (lower == "server") return Mode::Server;
    if (lower == "client") return Mode::Client;

    throw std::runtime_error("Unknown mode: " + s);
}

using namespace kut::context::io;
using namespace kut::context;

int main(const int argc, char **argv) {
    auto &log = Logger::instance();
    log.setLevel(LogLevel::DEBUG);

    const kut::cli::io::ArgumentParser parser(argc, argv);
    const std::string configPath = parser.getConfig();
    const Mode mode = parseMode(parser.getMode());

    switch (mode) {
        case Mode::Client: {
            log.info("Client mode selected");
            const std::unique_ptr<client::Context> context = std::make_unique<client::Context>(
                YamlReader::loadClientContextFromFile(configPath)
            );
            const kut::session::client::ClientSession clientSession(*context);
            clientSession.run();
            break;
        }
        case Mode::Server: {
            log.info("Server mode selected");
            const std::unique_ptr<server::Context> context = std::make_unique<
                server::Context>(
                YamlReader::loadServerContextFromFile(configPath)
            );
            const kut::session::server::ServerSession serverSession(*context);
            serverSession.run();
            break;
        }
    }
    return 0;
}