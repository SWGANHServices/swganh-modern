// src/core/logger.hpp
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

// Convenience macros
#define LOG_DEBUG(...) swganh::core::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) swganh::core::Logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) swganh::core::Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) swganh::core::Logger::instance().error(__VA_ARGS__)
#define LOG_FATAL(...) swganh::core::Logger::instance().fatal(__VA_ARGS__)

} // namespace swganh::core

// src/core/logger.cpp
#include "logger.hpp"
#include <chrono>
#include <iomanip>

namespace swganh::core {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::set_console_output(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_ = enabled;
}

void Logger::set_file_output(const string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_output_.is_open()) {
        file_output_.close();
    }
    
    file_output_.open(filename, std::ios::app);
    if (!file_output_.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::log(LogLevel level, const string& message) {
    log(level, "", message);
}

void Logger::log(LogLevel level, const string& category, const string& message) {
    if (level < min_level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    string timestamp = get_timestamp();
    string level_str = level_to_string(level);
    
    string formatted;
    if (category.empty()) {
        formatted = "[" + timestamp + "] [" + level_str + "] " + message;
    } else {
        formatted = "[" + timestamp + "] [" + level_str + "] [" + category + "] " + message;
    }
    
    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
    }
    
    if (file_output_.is_open()) {
        file_output_ << formatted << std::endl;
        file_output_.flush();
    }
}

string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace swganh::core