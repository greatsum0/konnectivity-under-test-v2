#pragma once

#include "itraffic_pattern.h"
#include "client_context.h"
#include <memory>
#include <stdexcept>
#include <string>
#include "constant/constant_traffic_pattern.h"

class TrafficPatternFactory {
public:
    static std::unique_ptr<ITrafficPattern> create(const ClientTrafficConfig& cfg) {
        const std::string& t = cfg.type;
        if (t == "constant") {
            return std::make_unique<ConstantTrafficPattern>(cfg.rateMbps);
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
        //         cfg.rateMbps,
        //         cfg.burstSizeMb,
        //         cfg.burstIntervalSec,
        //         cfg.burstDurationSec,
        //         static_cast<unsigned>(cfg.randomSeed),
        //         static_cast<unsigned>(cfg.minBurstSizeMb),
        //         static_cast<unsigned>(cfg.maxBurstSizeMb),
        //         static_cast<unsigned>(cfg.minBurstDurationMs),
        //         static_cast<unsigned>(cfg.maxBurstDurationMs),
        //         static_cast<unsigned>(cfg.minBurstIntervalMs),
        //         static_cast<unsigned>(cfg.maxBurstIntervalMs)
        //     );
        // }
        // else if (t == "sinusoidal") {
        //     return std::make_unique<SinusoidalTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.amplitude,
        //         cfg.frequency,
        //         cfg.intervalSec  // eğer pattern içinde zaman tabanlı hesaplak gerekiyorsa
        //     );
        // }
        // else if (t == "ramp_up") {
        //     return std::make_unique<RampUpTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.rampUpTime,
        //         cfg.durationSec,
        //         cfg.intervalSec
        //     );
        // }
        // else if (t == "ramp_down") {
        //     return std::make_unique<RampDownTrafficPattern>(
        //         cfg.rateMbps,
        //         cfg.rampDownTime,
        //         cfg.durationSec,
        //         cfg.intervalSec
        //     );
        // }
        // else if (t == "random") {
        //     return std::make_unique<RandomTrafficPattern>(
        //         static_cast<unsigned>(cfg.randomSeed),
        //         cfg.maxRateMbps,
        //         cfg.intervalSec
        //     );
        // }
        else {
            throw std::invalid_argument("Unknown traffic pattern type: " + t);
        }
    }
};
