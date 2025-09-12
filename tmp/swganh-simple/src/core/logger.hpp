#pragma once

#include "types.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <sstream>

namespace swganh::core {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& instance();
    
    void set_level(LogLevel level);
    void set_console_output(bool enabled);
    void set_file_output(const string& filename);
    
    void log(LogLevel level, const string& message);
    void log(LogLevel level, const string& category, const string& message);
    
    template<typename... Args>
    void debug(Args&&... args) {
        log_formatted(LogLevel::DEBUG, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(Args&&... args) {
        log_formatted(LogLevel::INFO, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(Args&&... args) {
        log_formatted(LogLevel::WARNING, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(Args&&... args) {
        log_formatted(LogLevel::ERROR, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void fatal(Args&&... args) {
        log_formatted(LogLevel::FATAL, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    
    template<typename... Args>
    void log_formatted(LogLevel level, Args&&... args) {
        if (level < min_level_) return;
        
        std::ostringstream oss;
        format_message(oss, std::forward<Args>(args)...);
        log(level, oss.str());
    }
    
    template<typename T>
    void format_message(std::ostringstream& oss, T&& arg) {
        oss << std::forward<T>(arg);
    }
    
    template<typename T, typename... Args>
    void format_message(std::ostringstream& oss, T&& first, Args&&... rest) {
        oss << std::forward<T>(first);
        format_message(oss, std::forward<Args>(rest)...);
    }
    
    string level_to_string(LogLevel level);
    string get_timestamp();
    
    LogLevel min_level_{LogLevel::DEBUG};
    bool console_output_{true};
    std::ofstream file_output_;
    std::mutex mutex_;
};

#define LOG_DEBUG(...) swganh::core::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) swganh::core::Logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) swganh::core::Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) swganh::core::Logger::instance().error(__VA_ARGS__)
#define LOG_FATAL(...) swganh::core::Logger::instance().fatal(__VA_ARGS__)

}
