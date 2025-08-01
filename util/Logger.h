#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <mutex>
#include <string>
#include <memory>
#include <iostream>
#include <atomic>

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Get the singleton instance
    static Logger& getInstance();

    // Set log file path (default: hft_sim.log)
    void setLogFile(const std::string& filename);

    // Enable/disable console/file sinks
    void enableConsole(bool enable);
    void enableFile(bool enable);

    // Set minimum log level
    void setLogLevel(LogLevel level);

    // Logging methods
    void log(LogLevel level, const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);

    // Deleted copy/move
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    void logImpl(LogLevel level, const std::string& msg);
    std::string levelToString(LogLevel level);

    
    std::ofstream file_stream_;
    std::mutex mutex_;
    size_t num_logged;
    std::array<std::string, 100> buffer;
    std::string log_file_ = "hft_sim.log";
    bool console_enabled_ = true;
    bool file_enabled_ = true;
    LogLevel min_level_ = LogLevel::DEBUG;
};

/*
 * NOTE: For high-performance (HFT) scenarios, I might upgrade to an asynchronous logger:
 * - Lock-free queue for log messages
 * - Have a dedicated logging thread to flush messages to sinks
 * - Support batching and configurable sinks
 */

#endif // LOGGER_H 