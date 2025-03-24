#pragma once

#include "abstract_burst_pattern.h"

class BurstTrafficPattern : public AbstractBurstPattern {
public:
    using SendFunction = std::function<void(const std::string &)>;

    BurstTrafficPattern(ClientContext &context, SendFunction sendFunc);

protected:
    std::string generatePayload() override;

private:
    int bytes_per_interval_ = 0;
    int burst_bytes_total_ = 0;
    int burst_bytes_per_interval_ = 0;
};
