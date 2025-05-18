#pragma once

// ————————————————————————————————————————————————————————————————————————————
// Logging macros (no external deps)
// ————————————————————————————————————————————————————————————————————————————
#define LOG_INFO(msg)  (std::cout  << "[INFO ] " << msg << std::endl)
#define LOG_WARN(msg)  (std::cerr  << "[WARN ] " << msg << std::endl)
#define LOG_ERROR(msg) (std::cerr  << "[ERROR]" << msg << std::endl)