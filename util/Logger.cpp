#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    num_logged = 0;
    if (file_enabled_) {
        file_stream_.open(log_file_, std::ios::app);
    }
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_ = filename;
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    if (file_enabled_) {
        file_stream_.open(log_file_, std::ios::app);
    }
}

void Logger::enableConsole(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_enabled_ = enable;
}

void Logger::enableFile(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_enabled_ = enable;
    if (file_enabled_ && !file_stream_.is_open()) {
        file_stream_.open(log_file_, std::ios::app);
    } else if (!file_enabled_ && file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < min_level_) return;
    logImpl(level, msg);
}

void Logger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, msg);
}
void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, msg);
}
void Logger::warning(const std::string& msg) {
    log(LogLevel::WARNING, msg);
}
void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, msg);
}

void Logger::logImpl(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    // Log format: [LEVEL][timestamp] message
    std::string log_line = "[" + levelToString(level) + "][" + ss.str() + "] " + msg + "\n";
    
    // buffer[num_logged] = log_line;
    // num_logged = (num_logged + 1) % buffer.size();
    if (console_enabled_ && num_logged == buffer.size() - 1) {
        std::cout << log_line;
    }
    if (file_enabled_ && file_stream_.is_open() && num_logged == buffer.size() - 1) {
        file_stream_ << log_line;
        file_stream_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}