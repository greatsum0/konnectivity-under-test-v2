#pragma once

#include <client_context.h>
#include <server_context.h>
#include <vector>
#include <yaml-cpp/node/node.h>

class YamlParser {
public:
    // Client
    static ClientContext parseClientContextFromFile(const std::string &path);

    static ClientContext parseClientContextFromString(const std::string &yamlText);

    static std::string toYamlString(const ClientContext &ctx);

    // Server

    static ServerContext parseServerContextFromFile(const std::string &path);

    static ServerContext parseServerContextFromString(const std::string &yamlText);

    static std::string toYamlString(const ServerContext &ctx);

private:
    // YAML loaders
    static YAML::Node loadYamlFromFile(const std::string &path);

    static YAML::Node parseYamlFromString(const std::string &yamlText);

    // Shared parsers
    static void parseGeneralSection(std::string &name, std::string &outputPath, const YAML::Node &node);

    static void parseMeasurementSection(MeasurementConfig &m, const YAML::Node &node);

    // Specific parsers
    static void parseClientNetwork(ClientNetworkConfig &net, const YAML::Node &node);

    static void parseServerNetwork(ServerNetworkConfig &net, const YAML::Node &node);

    static void parseTraffic(ClientTrafficConfig &traffic, const YAML::Node &node);
};
