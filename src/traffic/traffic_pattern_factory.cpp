#include "traffic_pattern_factory.h"
#include <stdexcept>
#include <iostream>

#include "burst/burst_traffic_pattern.h"
#include "burst/pure_burst_traffic_pattern.h"
#include "burst/random_burst_traffic_pattern.h"
#include "constant/constant_traffic_pattern.h"
#include "ramp/ramp_down_traffic_pattern.h"
#include "ramp/ramp_up_traffic_pattern.h"
#include "random/random_traffic_pattern.h"
#include "sinusoidal/sinusoidal_traffic_pattern.h"

std::shared_ptr<ITrafficPattern> TrafficPatternFactory::createTrafficPattern(
    ClientContext &ctx,
    const std::function<void(const std::string &)> &sendFunc) {
    const std::string &type = ctx.scenario.traffic_type;

    if (type == "constant") {
        return std::make_shared<ConstantTrafficPattern>(ctx, sendFunc);
    }
    if (type == "burst") {
        return std::make_shared<BurstTrafficPattern>(ctx, sendFunc);
    }
    if (type == "pure_burst") {
        return std::make_shared<PureBurstTrafficPattern>(ctx, sendFunc);
    }
    if (type == "random_burst") {
        return std::make_shared<RandomBurstTrafficPattern>(ctx, sendFunc);
    }
    if (type == "sinusoidal") {
        return std::make_shared<SinusoidalTrafficPattern>(ctx, sendFunc);
    }
    if (type == "random") {
        return std::make_shared<RandomTrafficPattern>(ctx, sendFunc);
    }
    if (type == "ramp_up") {
        return std::make_shared<RampUpTrafficPattern>(ctx, sendFunc);
    }
    if (type == "ramp_down") {
        return std::make_shared<RampDownTrafficPattern>(ctx, sendFunc);
    }

    std::cerr << "Unsupported traffic type: " << type << std::endl;
    throw std::invalid_argument("Unsupported traffic pattern: " + type);
}
