#include <iostream>
#include <string>
#include <asio.hpp>
#include <iomanip>

#include "yaml_parser.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "inetwork_client.h"
#include "arg_parser.h"
#include "client_context.h"

#include "server_context.h"
#include "traffic_pattern_factory.h"

void runServer(const std::string &configPath) {
    ServerConfig config = YamlParser::parseServerConfig(configPath);
    asio::io_context ioContext;
    ServerContext context(ioContext, config);

    std::unique_ptr<IServer> tcpServer;
    std::unique_ptr<IServer> udpServer;

    for (const auto &[protocol, _]: context.config.listeners) {
        if (protocol == "tcp") {
            tcpServer = std::make_unique<TcpServer>(context);
            tcpServer->run();
            std::cout << "TCP server running on port " << context.getPortForProtocol("tcp") << std::endl;
        } else if (protocol == "udp") {
            udpServer = std::make_unique<UdpServer>(context);
            udpServer->run();
            std::cout << "UDP server running on port " << context.getPortForProtocol("udp") << std::endl;
        }
    }

    context.ioContext.run();
}

void runClient(const std::string &configPath) {
    auto scenarios = YamlParser::parseScenarios(configPath);
    if (scenarios.empty()) {
        std::cerr << "No valid scenarios found in config file!" << std::endl;
        return;
    }

    asio::io_context ioContext;
    ClientContext ctx = ClientContext::fromScenario(ioContext, scenarios[0]);

    // appropriate client selection
    std::shared_ptr<INetworkClient> client;
    if (ctx.protocol() == "tcp") {
        auto tcpClient = std::make_shared<TcpClient>(ioContext, ctx.serverAddress(), ctx.serverPort());
        tcpClient->connect();  // connect for tcp
        client = tcpClient;
    } else if (ctx.protocol() == "udp") {
        client = std::make_shared<UdpClient>(ioContext, ctx.serverAddress(), ctx.serverPort());
    } else {
        std::cerr << "Unsupported protocol: " << ctx.protocol() << std::endl;
        return;
    }

    // Sending traffic function
    auto sendFunc = [client](const std::string &data) {
        client->sendMessage(data);
    };

    // create the traffic pattern and start it
    auto trafficPattern = TrafficPatternFactory::createTrafficPattern(ctx, sendFunc);
    if (!trafficPattern) {
        std::cerr << "Failed to create traffic pattern for type: " << ctx.scenario.traffic_type << std::endl;
        return;
    }

    trafficPattern->start();
    ioContext.run();
}



int main(int argc, char *argv[]) {
    ArgumentParser parser(argc, argv);

    if (parser.shouldShowHelp()) {
        std::cout << ArgumentParser::usageMessage() << std::endl;
        return 0;
    }

    std::string mode = parser.getMode();
    std::string configPath = parser.getConfig();

    if (mode == "server") {
        std::cout << "Starting server mode..." << std::endl;
        runServer(configPath);
    } else if (mode == "client") {
        std::cout << "Starting client mode..." << std::endl;
        runClient(configPath);
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }

    return 0;
}
