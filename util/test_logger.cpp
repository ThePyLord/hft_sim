#include "Logger.h"
#include <thread>
#include <filesystem>
#include <vector>
#include <gtest/gtest.h>

class LoggingTest : public ::testing::Test {
protected:
    LoggingTest() {
        Logger& logger = Logger::getInstance();
        logger.setLogFile("test_logger.log");
        logger.setLogLevel(LogLevel::DEBUG);
        logger.enableConsole(true);
        logger.enableFile(true);
    }

    void TearDown() override {
        using namespace std::filesystem;
        // Clean up the log file after each test
        if (exists("test_logger.log")) {
            std::cout << "Removing test_logger.log" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait for 5 seconds to ensure all logs are written
            remove("test_logger.log");
            std::cout << "Removed test_logger.log" << std::endl;
        }

        // remove("test_logger.log");
    }
};

TEST_F(LoggingTest, SingleThreadedLogging) {
    Logger& logger = Logger::getInstance();
    logger.enableFile(true);
    logger.info("Single-threaded test started");
    logger.debug("This is a debug message");
    logger.warning("This is a warning message");
    logger.error("This is an error message");

    // Check if the log file was created and contains expected messages
    std::ifstream log_file("test_logger.log");
    ASSERT_TRUE(log_file.is_open());
    std::string line;
    bool found_info = false;
    while (std::getline(log_file, line)) {
        if (line.find("Single-threaded test started") != std::string::npos) {
            found_info = true;
            break;
        }
    }
    ASSERT_TRUE(found_info);
}
