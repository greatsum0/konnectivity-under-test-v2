#pragma once

#include "traffic_pattern.h"
#include "client_context.h"
#include <asio.hpp>
#include <functional>
#include <memory>

class AbstractRampPattern : public ITrafficPattern, public std::enable_shared_from_this<AbstractRampPattern> {
public:
    using SendFunction = std::function<void(const std::string&)>;

    AbstractRampPattern(ClientContext& context, SendFunction sendFunc);

    void start() override;

protected:
    virtual int calculateRateMbps(int elapsed_ms) = 0;

    void scheduleNextSend();

    ClientContext& context_;
    SendFunction sendFunc_;
    asio::steady_timer timer_;
    int interval_ms_;
    int elapsed_ms_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point nextPlannedTime_;
};
