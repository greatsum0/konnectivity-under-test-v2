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
        ClientContext clientCtx = YamlParser::parseClientContext(configPath);
        ClientMode clientApp(clientCtx);
        clientApp.run();
    } else if (mode == "server") {
        ServerContext serverCtx = YamlParser::parseServerContext(configPath);
        ServerMode serverApp(serverCtx);
        serverApp.run();
    } else {
        std::cerr << "Invalid mode: " << mode << "\n";
        return 1;
    }

    return 0;
}
