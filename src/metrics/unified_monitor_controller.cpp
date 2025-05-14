#include "unified_monitor_controller.h"

UnifiedMonitorController::UnifiedMonitorController(
    asio::io_context &ioContext,
    std::shared_ptr<CPUUsageMonitor> cpuMonitor,
    std::shared_ptr<PortTrafficMonitor> networkMonitor,
    const int intervalSec
)
    : ioContext_(ioContext),
      cpuMonitor_(std::move(cpuMonitor)),
      networkMonitor_(std::move(networkMonitor)),
      intervalSec_(intervalSec),
      timer_(std::make_unique<asio::steady_timer>(ioContext_)) {
}

void UnifiedMonitorController::start(Callback callback) {
    if (running_) return;
    running_ = true;
    callback_ = std::move(callback);
    tick();
}

void UnifiedMonitorController::stop() {
    running_ = false;
    timer_->cancel();
}

void UnifiedMonitorController::tick() {
    if (!running_) return;

    const auto now = Clock::now();

    const CPUUsageSnapshot cpuSnapshot = cpuMonitor_->sample();
    const PortTrafficMonitor::AggregateSample netSample = networkMonitor_->sample();

    callback_(now, cpuSnapshot, netSample);

    timer_->expires_after(std::chrono::seconds(intervalSec_));
    timer_->async_wait([this](const asio::error_code &ec) {
        if (!ec) tick();
    });
}
