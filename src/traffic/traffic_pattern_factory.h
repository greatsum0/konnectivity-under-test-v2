#pragma once

#include "traffic_pattern.h"
#include "client_context.h"
#include <memory>
#include <functional>
#include <string>

class TrafficPatternFactory {
public:
    static std::shared_ptr<ITrafficPattern> createTrafficPattern(
        ClientContext& ctx,
        const std::function<void(const std::string&)>& sendFunc);
};
