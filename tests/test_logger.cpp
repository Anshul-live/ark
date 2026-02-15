#include <gtest/gtest.h>
#include <logger.h>
#include <fstream>
#include <filesystem>
#include <sstream>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testLogFile = std::filesystem::temp_directory_path() / "ark_test_log.txt";
        std::filesystem::remove(testLogFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(testLogFile);
    }
    
    std::filesystem::path testLogFile;
};

TEST_F(LoggerTest, InitializeLogger) {
    Logger::instance().init(testLogFile.string());
    EXPECT_TRUE(std::filesystem::exists(testLogFile));
}

TEST_F(LoggerTest, ConsoleOutput) {
    Logger::instance().init(testLogFile.string());
    Logger::instance().setConsoleOutput(true);
    Logger::instance().info("Console message");
    SUCCEED();
}

TEST_F(LoggerTest, LogLevels) {
    Logger::instance().init(testLogFile.string());
    Logger::instance().setConsoleOutput(true);
    Logger::instance().debug("Debug");
    Logger::instance().info("Info");
    Logger::instance().warning("Warning");
    Logger::instance().error("Error");
    SUCCEED();
}

TEST_F(LoggerTest, LevelFiltering) {
    Logger::instance().init(testLogFile.string());
    Logger::instance().setConsoleOutput(true);
    Logger::instance().setLevel(LogLevel::WARNING);
    Logger::instance().debug("Debug - should not show");
    Logger::instance().info("Info - should not show");
    Logger::instance().warning("Warning - should show");
    Logger::instance().error("Error - should show");
    SUCCEED();
}

TEST_F(LoggerTest, SetLevel) {
    Logger::instance().setLevel(LogLevel::DEBUG);
    Logger::instance().setLevel(LogLevel::INFO);
    Logger::instance().setLevel(LogLevel::WARNING);
    Logger::instance().setLevel(LogLevel::ERROR);
    SUCCEED();
}

TEST_F(LoggerTest, MultipleLogCalls) {
    Logger::instance().info("First");
    Logger::instance().info("Second");
    Logger::instance().info("Third");
    SUCCEED();
}

TEST_F(LoggerTest, FatalLevel) {
    Logger::instance().init(testLogFile.string());
    Logger::instance().setConsoleOutput(true);
    Logger::instance().fatal("Fatal error");
    SUCCEED();
}
