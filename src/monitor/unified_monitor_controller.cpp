#include "unified_monitor_controller.h"

#include <iostream>

UnifiedMonitorController::UnifiedMonitorController(
    boost::asio::io_context &ioContext,
    std::shared_ptr<CPUUsageMonitor> cpuMonitor,
    std::shared_ptr<PortTrafficMonitor> networkMonitor,
    const double intervalSec
)
    : ioContext_(ioContext),
      cpuMonitor_(std::move(cpuMonitor)),
      networkMonitor_(std::move(networkMonitor)),
      intervalSec_(intervalSec),
      timer_(std::make_unique<boost::asio::steady_timer>(ioContext_)) {
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
    const auto now = std::chrono::system_clock::now();
    const CPUUsageSnapshot cpuSnapshot = cpuMonitor_->sample();
    const AggregateSample netSample = networkMonitor_->sample();

    callback_(now, cpuSnapshot, netSample);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - now
    );
    const auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double>(intervalSec_)
    );
    const auto waitMs = (intervalMs > elapsed) ? (intervalMs - elapsed) : std::chrono::milliseconds(0);
    timer_->expires_after(waitMs);
    timer_->async_wait([this](const boost::system::error_code &ec) {
        if (!ec) tick();
    });
}
