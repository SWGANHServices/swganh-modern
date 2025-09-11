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

}
