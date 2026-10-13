#include "support/ark_test.h"

#include <logger.h>

using namespace arktest;

// Logger is a process-wide singleton whose init() only takes effect once, so
// everything that needs the log file lives in a single test.
TEST(Logger, WritesLevelTaggedLinesAndFiltersBelowMinimum) {
  TempDir tmp;
  const fs::path log = tmp.path() / "logs/ark.log";
  Logger &logger = Logger::instance();
  logger.init(log.string());
  logger.setConsoleOutput(false);
  logger.setLevel(LogLevel::WARNING);

  logger.debug("debug-hidden");
  logger.info("info-hidden");
  logger.warning("warning-shown");
  logger.error("error-shown");
  logger.fatal("fatal-shown");
  logger.setLevel(LogLevel::INFO);

  ASSERT_TRUE(fs::exists(log)) << "init() should create parent directories";
  const std::string text = readFile(log);
  EXPECT_EQ(text.find("debug-hidden"), std::string::npos);
  EXPECT_EQ(text.find("info-hidden"), std::string::npos);
  EXPECT_NE(text.find("[WARN] warning-shown"), std::string::npos) << text;
  EXPECT_NE(text.find("[ERROR] error-shown"), std::string::npos) << text;
  EXPECT_NE(text.find("[FATAL] fatal-shown"), std::string::npos) << text;
}

// tmp/bugs/17: diagnostics must never go to stdout, which belongs to command output.
TEST(Logger, ConsoleOutputNeverGoesToStdout) {
  Logger &logger = Logger::instance();
  logger.setConsoleOutput(true);
  logger.setLevel(LogLevel::DEBUG);
  ::testing::internal::CaptureStdout();
  logger.debug("d");
  logger.info("i");
  logger.warning("w");
  const std::string out = ::testing::internal::GetCapturedStdout();
  logger.setConsoleOutput(false);
  logger.setLevel(LogLevel::INFO);
  EXPECT_EQ(out, "");
}
