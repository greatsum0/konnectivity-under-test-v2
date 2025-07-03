// utils/logger.h
#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// Log levels
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
public:
    // Get the singleton instance
    static Logger &instance() {
        static Logger inst;
        return inst;
    }

    // Change the minimum level to output
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    // General logging method
    template<typename... Args>
    void log(LogLevel lvl, Args &&... args) {
        if (lvl < level_) return;

        // Build the message
        std::ostringstream oss;
        oss << timestamp()
                << " [" << levelToString(lvl) << "] "
                << "[T" << std::this_thread::get_id() << "] ";

        // Pack all args into the stream
        (oss << ... << std::forward<Args>(args));

        // Thread-safe output
        std::lock_guard<std::mutex> lock(mutex_);
        auto &out = (lvl == LogLevel::ERROR ? std::cerr : std::cout);
        out << levelToColor(lvl)
                << oss.str()
                << colorReset()
                << std::endl;
    }

    // Convenience wrappers
    template<typename... Args>
    void debug(Args &&... args) { log(LogLevel::DEBUG, std::forward<Args>(args)...); }

    template<typename... Args>
    void info(Args &&... args) { log(LogLevel::INFO, std::forward<Args>(args)...); }

    template<typename... Args>
    void warning(Args &&... args) { log(LogLevel::WARNING, std::forward<Args>(args)...); }

    template<typename... Args>
    void error(Args &&... args) { log(LogLevel::ERROR, std::forward<Args>(args)...); }

private:
    Logger() : level_(LogLevel::DEBUG) {
    }

    // Format current time as YYYY-MM-DD HH:MM:SS
    std::string timestamp() const {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto tt = system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&tt, &tm);

        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Map level → string
    static const char *levelToString(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO ";
            case LogLevel::WARNING: return "WARN ";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKWN";
    }

    // ANSI color codes per level
    static const char *levelToColor(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::DEBUG: return "\033[36m"; // Cyan
            case LogLevel::INFO: return "\033[32m"; // Green
            case LogLevel::WARNING: return "\033[33m"; // Yellow
            case LogLevel::ERROR: return "\033[31m"; // Red
        }
        return "\033[0m";
    }

    static const char *colorReset() { return "\033[0m"; }

    mutable std::mutex mutex_;
    LogLevel level_;
};
