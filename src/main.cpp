#include <iostream>
#include <string>
#include "client_mode.h"
#include "server_mode.h"
#include "yaml_parser.h"
#include "arg_parser.h"
#include "client_context.h"
#include "server_context.h"


int main(int argc, char **argv) {
    ArgumentParser parser(argc, argv);
    std::string configPath = parser.getConfig();
    std::string mode = parser.getMode(); // "client" or "server"

    if (mode == "client") {
        auto clientCtx = std::make_unique<ClientContext>(
            YamlParser::parseClientContextFromFile(configPath)
        );
        ClientMode clientApp(*clientCtx);
        clientApp.run();
    } else if (mode == "server") {
        auto serverCtx = std::make_unique<ServerContext>(
            YamlParser::parseServerContextFromFile(configPath)
        );
        ServerMode serverApp(*serverCtx);
        serverApp.run();
    } else {
        std::cerr << "[ERROR] Invalid mode: " << mode << "\n";
        return 1;
    }

    return 0;
}
