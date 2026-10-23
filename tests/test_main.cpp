#include <gtest/gtest.h>
#include <ark/util/logger.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  // Keep in-process unit test output readable; logger tests turn it back on.
  Logger::instance().setConsoleOutput(false);
  return RUN_ALL_TESTS();
}
