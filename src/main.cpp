#include <arg_parser.h>
#include <client_context.h>
#include <client_session.h>
#include <iostream>
#include <logger.h>
#include <server_session.h>
#include <string>
#include <yaml_parser.h>

enum class Mode { Server, Client };

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

int main(const int argc, char **argv) {
    auto &log = Logger::instance();
    log.setLevel(LogLevel::DEBUG);

    try {
        const ArgumentParser parser(argc, argv);
        const std::string configPath = parser.getConfig();

        if (const Mode mode = parseMode(parser.getMode()); mode == Mode::Client) {
            log.info("Running in client mode");
            const auto clientCtx = std::make_unique<ClientContext>(
                YamlParser::parseClientContextFromFile(configPath)
            );
            ClientSession clientSession(*clientCtx);
            return clientSession.run();
        }
        log.info("Running in server mode");
        while (true) {
            log.info("Starting a new session");
            const auto serverCtx = std::make_unique<ServerContext>(
                YamlParser::parseServerContextFromFile(configPath)
            );
            ServerSession serverSession(*serverCtx);
            const int success = serverSession.run();
            log.info("Session finished with code: " + std::to_string(success));
        }
    } catch (const std::exception &e) {
        log.error("Exception: " + std::string(e.what()));
        return 1;
    }
}
