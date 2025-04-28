#pragma once

#include "itraffic_pattern.h"
#include "client_context.h"
#include <memory>
#include <stdexcept>
#include <string>

#include "traffic/constant/constant_traffic_pattern.h"
// #include "traffic/burst/burst_traffic_pattern.h"
// #include "traffic/burst/pure_burst_traffic_pattern.h"
// #include "traffic/burst/random_burst_traffic_pattern.h"
// #include "traffic/sinusoidal/sinusoidal_traffic_pattern.h"
// #include "traffic/ramp/ramp_up_traffic_pattern.h"
// #include "traffic/ramp/ramp_down_traffic_pattern.h"
// #include "traffic/random/random_traffic_pattern.h"


class TrafficPatternFactory {
public:
    static std::unique_ptr<ITrafficPattern> create(const ClientTrafficConfig& cfg) {
        const auto& t = cfg.type;

        if (t == "constant") {
            return std::make_unique<ConstantTrafficPattern>(
                cfg.rateMbps
            );
        }
        // else if (t == "burst") {
        //     return std::make_unique<BurstTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.burstSizeMb,
        //         cfg.burstDurationSec,
        //         cfg.burstIntervalSec
        //     );
        // }
        // else if (t == "pure_burst") {
        //     return std::make_unique<PureBurstTrafficPattern>(
        //         cfg.burstSizeMb,
        //         cfg.burstDurationSec,
        //         cfg.burstIntervalSec
        //     );
        // }
        // else if (t == "random_burst") {
        //     return std::make_unique<RandomBurstTrafficPattern>(
        //         static_cast<unsigned>(cfg.randomSeed),
        //         cfg.rateMbps,
        //         cfg.minBurstSizeMb,
        //         cfg.maxBurstSizeMb,
        //         cfg.minBurstDurationMs,
        //         cfg.maxBurstDurationMs,
        //         cfg.minBurstIntervalMs,
        //         cfg.maxBurstIntervalMs
        //     );
        // }
        // else if (t == "sinusoidal") {
        //     return std::make_unique<SinusoidalTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.amplitude,
        //         cfg.frequency
        //     );
        // }
        // else if (t == "ramp_up") {
        //     return std::make_unique<RampUpTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.rampUpTime
        //     );
        // }
        // else if (t == "ramp_down") {
        //     return std::make_unique<RampDownTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.rampDownTime
        //     );
        // }
        // else if (t == "random") {
        //     return std::make_unique<RandomTrafficPattern>(
        //         static_cast<unsigned>(cfg.randomSeed),
        //         cfg.maxRateMbps
        //     );
        // }
        else {
            throw std::invalid_argument("Unknown traffic pattern type: " + t);
        }
    }
};
