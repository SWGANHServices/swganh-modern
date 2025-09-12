// File: src/core/logger.hpp
#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>

namespace swganh {

enum class LogLevel {
    DEBUG_LEVEL = 0,
    INFO_LEVEL = 1,
    WARNING_LEVEL = 2,
    ERROR_LEVEL = 3,
    FATAL_LEVEL = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }
    
    void set_level(LogLevel level) {
        min_level_ = level;
    }
    
    void log(LogLevel level, const std::string& message) {
        if (level < min_level_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string level_str = level_to_string(level);
        std::cout << "[" << level_str << "] " << message << std::endl;
    }
    
    // Template for formatted logging
    template<typename... Args>
    void log_formatted(LogLevel level, const std::string& format, Args... args) {
        if (level < min_level_) return;
        
        std::ostringstream oss;
        format_impl(oss, format, args...);
        log(level, oss.str());
    }

private:
    Logger() = default;
    
    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG_LEVEL:   return "DEBUG";
            case LogLevel::INFO_LEVEL:    return "INFO";
            case LogLevel::WARNING_LEVEL: return "WARN";
            case LogLevel::ERROR_LEVEL:   return "ERROR";
            case LogLevel::FATAL_LEVEL:   return "FATAL";
            default:                      return "UNKNOWN";
        }
    }
    
    // Simple string formatting
    void format_impl(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
    
    template<typename T, typename... Args>
    void format_impl(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value;
            format_impl(oss, format.substr(pos + 2), args...);
        } else {
            oss << format;
        }
    }
    
    LogLevel min_level_{LogLevel::DEBUG_LEVEL};
    std::mutex mutex_;
};

// Simple initialization
inline void init_logger(LogLevel level = LogLevel::DEBUG_LEVEL) {
    Logger::instance().set_level(level);
}

} // namespace swganh

// Simple macros
#define LOG_DEBUG(msg) swganh::Logger::instance().log(swganh::LogLevel::DEBUG_LEVEL, msg)
#define LOG_INFO(msg) swganh::Logger::instance().log(swganh::LogLevel::INFO_LEVEL, msg)
#define LOG_WARNING(msg) swganh::Logger::instance().log(swganh::LogLevel::WARNING_LEVEL, msg)
#define LOG_ERROR(msg) swganh::Logger::instance().log(swganh::LogLevel::ERROR_LEVEL, msg)
#define LOG_FATAL(msg) swganh::Logger::instance().log(swganh::LogLevel::FATAL_LEVEL, msg)

// Formatted logging macros
#define LOG_DEBUG_F(fmt, ...) swganh::Logger::instance().log_formatted(swganh::LogLevel::DEBUG_LEVEL, fmt, __VA_ARGS__)
#define LOG_INFO_F(fmt, ...) swganh::Logger::instance().log_formatted(swganh::LogLevel::INFO_LEVEL, fmt, __VA_ARGS__)
#define LOG_WARNING_F(fmt, ...) swganh::Logger::instance().log_formatted(swganh::LogLevel::WARNING_LEVEL, fmt, __VA_ARGS__)
#define LOG_ERROR_F(fmt, ...) swganh::Logger::instance().log_formatted(swganh::LogLevel::ERROR_LEVEL, fmt, __VA_ARGS__)
#define LOG_FATAL_F(fmt, ...) swganh::Logger::instance().log_formatted(swganh::LogLevel::FATAL_LEVEL, fmt, __VA_ARGS__)