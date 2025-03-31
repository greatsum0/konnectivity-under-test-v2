#pragma once

#include "traffic_pattern.h"
#include <asio/steady_timer.hpp>
#include <functional>
#include <memory>

// Forward declarations
class ClientContext;

class AbstractBurstPattern : public ITrafficPattern, public std::enable_shared_from_this<AbstractBurstPattern> {
public:
    using SendFunction = std::function<void(const std::string&)>;

    AbstractBurstPattern(ClientContext& context, SendFunction sendFunc);
    void start() override;

protected:
    virtual std::string generatePayload() = 0;
    void scheduleNextSend();

    ClientContext& context_;
    SendFunction sendFunc_;
    asio::steady_timer timer_;
    int interval_ms_;
    int elapsed_ms_;
    int burst_interval_ms_ = 0;
    int burst_duration_ms_ = 0;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point nextPlannedTime_;

};
