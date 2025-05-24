// abstract_burst_pattern.cpp
#include "abstract_burst_pattern.h"

AbstractBurstPattern::AbstractBurstPattern()

    : startTime_(std::chrono::steady_clock::now()) {
}
