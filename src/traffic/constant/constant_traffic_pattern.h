#pragma once

#include "client_context.h"
#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>

#include "traffic_pattern.h"


class ConstantTrafficPattern : public ITrafficPattern, public std::enable_shared_from_this<ConstantTrafficPattern> {
public:
    using SendFunction = std::function<void(const std::string&)>;

    ConstantTrafficPattern(ClientContext& context, SendFunction sendFunc);

    void start() override;

private:
    void sendData();

    ClientContext& context_;
    SendFunction sendFunc_;
    asio::steady_timer timer_;
    int interval_ms_;
    int elapsed_ms_;
    int bytes_per_interval_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point nextPlannedTime_;

};
