#pragma once

#include <client_context.h>
#include <server_context.h>
#include <vector>

class YamlParser {
public:
    static ServerContext parseServerContext(const std::string &path);

    static ClientContext parseClientContext(const std::string &path);
};


