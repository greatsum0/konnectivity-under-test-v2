#include "arg_parser.h"
#include <iostream>
#include <getopt.h>
#include <stdexcept>

ArgumentParser::ArgumentParser(const int argc, char *argv[]) {
    const auto short_opts = "m:c:h";
    constexpr option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"config", required_argument, nullptr, 'c'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {
        switch (opt) {
            case 'm':
                mode = optarg;
                if (mode != "server" && mode != "client") {
                    throw std::invalid_argument(
                        "Invalid mode: " + mode + ". Must be 'server' or 'client'.\n" + usageMessage());
                }
                break;

            case 'c':
                config = optarg;
                break;

            case 'h':
                showHelp = true;
                break;

            default:
                throw std::invalid_argument("Unknown argument.\n" + usageMessage());
        }
    }

    // Validate required arguments
    if (mode.empty()) {
        throw std::invalid_argument("Missing required argument: --mode.\n" + usageMessage());
    }

    if (config.empty()) {
        throw std::invalid_argument("Missing required argument: --config.\n" + usageMessage());
    }
}

std::string ArgumentParser::usageMessage() {
    return "Usage: program_name -m <server|client> -p <port> [options]\n"
            "Options:\n"
            "  -m, --mode <server|client>    Mode of operation (required)\n"
            "  -c, --config <file>           Configuration file (required)\n"
            "  -h, --help                   Show this help message";
}

// Accessor functions
std::string ArgumentParser::getMode() const { return mode; }
std::string ArgumentParser::getConfig() const { return config; }
bool ArgumentParser::shouldShowHelp() const { return showHelp; }
