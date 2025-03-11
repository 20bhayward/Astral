#include "astral/core/Logger.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <regex>

namespace astral {
namespace test {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set test log file path
        testLogPath = "test_log.txt";
        
        // Remove test log file if it exists
        if (std::filesystem::exists(testLogPath)) {
            std::filesystem::remove(testLogPath);
        }
    }
    
    void TearDown() override {
        // Clean up test log file
        if (std::filesystem::exists(testLogPath)) {
            std::filesystem::remove(testLogPath);
        }
    }
    
    // Helper to check if log file contains a pattern
    bool logFileContains(const std::string& pattern) {
        if (!std::filesystem::exists(testLogPath)) {
            return false;
        }
        
        std::ifstream file(testLogPath);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        
        std::regex regex(pattern);
        return std::regex_search(content, regex);
    }
    
    std::string testLogPath;
};

TEST_F(LoggerTest, Constructor) {
    Logger logger("TestLogger");
    // Nothing to assert here, just check it constructs
}

TEST_F(LoggerTest, Initialize) {
    Logger logger("TestLogger");
    
    // Test initialization
    EXPECT_TRUE(logger.initialize(testLogPath));
    
    // Check log file was created
    EXPECT_TRUE(std::filesystem::exists(testLogPath));
}

TEST_F(LoggerTest, LogLevels) {
    Logger logger("TestLogger");
    
    // Initialize logger with test file
    EXPECT_TRUE(logger.initialize(testLogPath));
    
    // Test different log levels
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warn("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");
    
    // Check log file contains messages
    EXPECT_TRUE(logFileContains("Debug message"));
    EXPECT_TRUE(logFileContains("Info message"));
    EXPECT_TRUE(logFileContains("Warning message"));
    EXPECT_TRUE(logFileContains("Error message"));
    EXPECT_TRUE(logFileContains("Critical message"));
    
    // Check log level indicators
    EXPECT_TRUE(logFileContains("\\[debug\\].*Debug message"));
    EXPECT_TRUE(logFileContains("\\[info\\].*Info message"));
    EXPECT_TRUE(logFileContains("\\[warning\\].*Warning message"));
    EXPECT_TRUE(logFileContains("\\[error\\].*Error message"));
    EXPECT_TRUE(logFileContains("\\[critical\\].*Critical message"));
}

} // namespace test
} // namespace astral