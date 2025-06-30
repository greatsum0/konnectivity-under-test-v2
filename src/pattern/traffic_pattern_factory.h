#pragma once

#include "itraffic_pattern.h"
#include "context.h"
#include <memory>
#include <stdexcept>
#include <string>

#include "constant_traffic_pattern.h"
#include "burst_traffic_pattern.h"
#include "pure_burst_traffic_pattern.h"
#include "random_burst_traffic_pattern.h"
#include "sinusoidal_traffic_pattern.h"
#include "ramp_up_traffic_pattern.h"
#include "ramp_down_traffic_pattern.h"


class TrafficPatternFactory {
public:
    static std::unique_ptr<ITrafficPattern> create(const kut::context::traffic::Config &trafficConfig) {
        const auto &t = trafficConfig;

        if (t.type() == "constant") {
            return std::make_unique<ConstantTrafficPattern>(
                trafficConfig.constant().rate_mbps()
            );
        }
        if (t.type() == "burst") {
            return std::make_unique<BurstTrafficPattern>(
                trafficConfig.burst().rate_mbps(),
                trafficConfig.burst().burst_size_mb(),
                trafficConfig.burst().burst_duration_sec(),
                trafficConfig.burst().burst_interval_sec()
            );
        }
        if (t.type() == "pure_burst") {
            return std::make_unique<PureBurstTrafficPattern>(
                trafficConfig.pure_burst().burst_size_mb(),
                trafficConfig.pure_burst().burst_duration_sec(),
                trafficConfig.pure_burst().burst_interval_sec()
            );
        }
        if (t.type() == "random_burst") {
            return std::make_unique<RandomBurstTrafficPattern>(
                trafficConfig.random_burst().rate_mbps(),
                trafficConfig.random_burst().min_burst_size_mb(),
                trafficConfig.random_burst().max_burst_size_mb(),
                trafficConfig.random_burst().min_burst_duration_ms(),
                trafficConfig.random_burst().max_burst_duration_ms(),
                trafficConfig.random_burst().min_burst_interval_ms(),
                trafficConfig.random_burst().max_burst_interval_ms(),
                static_cast<unsigned>(trafficConfig.random_burst().random_seed())
            );
        }
        if (t.type() == "sinusoidal") {
            return std::make_unique<SinusoidalTrafficPattern>(
                trafficConfig.sinusoidal().rate_mbps(),
                trafficConfig.sinusoidal().amplitude(),
                trafficConfig.sinusoidal().frequency()
            );
        }
        if (t.type() == "ramp_up") {
            return std::make_unique<RampUpTrafficPattern>(
                trafficConfig.ramp_up().rate_mbps(),
                trafficConfig.ramp_up().ramp_up_time()
            );
        }
        if (t.type() == "ramp_down") {
            return std::make_unique<RampDownTrafficPattern>(
                trafficConfig.ramp_down().rate_mbps(),
                trafficConfig.ramp_down().ramp_down_time()
            );
        }
        throw std::invalid_argument("Unknown traffic pattern type: " + t.type());
    }
};
