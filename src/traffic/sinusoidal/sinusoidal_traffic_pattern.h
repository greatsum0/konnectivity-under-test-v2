#pragma once

#include "traffic_pattern.h"
#include "client_context.h"
#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>

class SinusoidalTrafficPattern : public ITrafficPattern, public std::enable_shared_from_this<SinusoidalTrafficPattern> {
public:
    using SendFunction = std::function<void(const std::string&)>;

    SinusoidalTrafficPattern(ClientContext& context, SendFunction sendFunc);

    void start() override;

private:
    void sendData();

    ClientContext& context_;
    SendFunction sendFunc_;
    asio::steady_timer timer_;

    // Sinusoidal parameters
    double amplitude_;     // Mbps
    double frequency_;     // Hz
    double base_rate_;     // center of the wave rate, Mbps

    int interval_ms_;      // (ms) send interval
    int elapsed_ms_;       // (ms) total elapsed time
};
