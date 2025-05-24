#pragma once

#include "itraffic_pattern.h"
#include "client_context.h"
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
    static std::unique_ptr<ITrafficPattern> create(const ClientTrafficConfig &cfg) {
        const auto &t = cfg.type;

        if (t == "constant") {
            return std::make_unique<ConstantTrafficPattern>(
                cfg.rateMbps
            );
        }
        if (t == "burst") {
            return std::make_unique<BurstTrafficPattern>(
                cfg.rateMbps,
                cfg.burstSizeMb,
                cfg.burstDurationSec,
                cfg.burstIntervalSec
            );
        }
        if (t == "pure_burst") {
            return std::make_unique<PureBurstTrafficPattern>(
                cfg.burstSizeMb,
                cfg.burstDurationSec,
                cfg.burstIntervalSec
            );
        }
        if (t == "random_burst") {
            return std::make_unique<RandomBurstTrafficPattern>(
                cfg.rateMbps,
                cfg.minBurstSizeMb,
                cfg.maxBurstSizeMb,
                cfg.minBurstDurationMs,
                cfg.maxBurstDurationMs,
                cfg.minBurstIntervalMs,
                cfg.maxBurstIntervalMs,
                static_cast<unsigned>(cfg.randomSeed)
            );
        }
        if (t == "sinusoidal") {
            auto &parameter = cfg;
            double mean = parameter.rateMbps; // YAML: rate_mbps
            double amplitude = parameter.amplitude; // YAML: amplitude
            double frequency = parameter.frequency; // YAML: frequency (Hz)

            return std::make_unique<SinusoidalTrafficPattern>(
                mean, amplitude, frequency
            );
        }
        if (t == "ramp_up") {
            return std::make_unique<RampUpTrafficPattern>(
                cfg.rateMbps,
                cfg.rampUpTime
            );
        }
        if (t == "ramp_down") {
            return std::make_unique<RampDownTrafficPattern>(
                cfg.rateMbps,
                cfg.rampDownTime
            );
        }
        throw std::invalid_argument("Unknown traffic pattern type: " + t);
    }
};
