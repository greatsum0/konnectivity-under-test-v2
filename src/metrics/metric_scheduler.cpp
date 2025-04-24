#include "metric_scheduler.h"
#include <chrono>

MetricScheduler::MetricScheduler(asio::io_context &io,
                                 MetricCollector &collector,
                                 double intervalSec)
    : timer_(io),
      collector_(collector),
      nextTime_(std::chrono::steady_clock::now()),
      interval_(intervalSec) {
}

void MetricScheduler::start() {
    scheduleNext();
}

void MetricScheduler::scheduleNext() {
    nextTime_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval_);
    timer_.expires_at(nextTime_);
    timer_.async_wait([this](const asio::error_code &ec) {
        if (!ec) {
            collector_.collect();
            scheduleNext();
        }
    });
}
