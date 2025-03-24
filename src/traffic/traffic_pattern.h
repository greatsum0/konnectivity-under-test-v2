#pragma once

class ITrafficPattern {
public:
    virtual void start() = 0;
    virtual ~ITrafficPattern() = default;
};
