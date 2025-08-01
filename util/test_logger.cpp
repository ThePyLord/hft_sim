#include "Logger.h"
#include <thread>
#include <vector>

void log_from_thread(int thread_id) {
    for (int i = 0; i < 5; ++i) {
        Logger::getInstance().info("Thread " + std::to_string(thread_id) + " logging info " + std::to_string(i));
        Logger::getInstance().debug("Thread " + std::to_string(thread_id) + " debug " + std::to_string(i));
    }
}

int main() {
    Logger& logger = Logger::getInstance();

    logger.setLogFile("test_logger.log");
    logger.setLogLevel(LogLevel::DEBUG);
    logger.enableConsole(true);
    logger.enableFile(true);

    logger.info("Logger test started");
    logger.debug("This is a debug message");
    logger.warning("This is a warning message");
    logger.error("This is an error message");

    // Test multi-threaded logging
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(log_from_thread, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    logger.info("Logger test finished");
    return 0;
} 