#include "support/ark_test.h"

using namespace arktest;

TEST(Init, CreatesRepositoryLayout) {
  Sandbox sb;
  RunResult r = sb.ark({"init"});
  ASSERT_EQ(r.exitCode, 0) << r;

  const fs::path ark = sb.repo() / ".ark";
  EXPECT_TRUE(fs::is_directory(ark / "objects"));
  EXPECT_TRUE(fs::is_directory(ark / "refs/heads"));
  EXPECT_TRUE(fs::is_regular_file(ark / "index"));
  EXPECT_TRUE(fs::is_regular_file(ark / "config"));
  EXPECT_EQ(sb.headFile(), "ref: refs/heads/main");
  EXPECT_EQ(sb.headCommit(), "") << "a fresh repository has no commits";
}

TEST(Init, RunningInitAgainKeepsExistingData) {
  Sandbox sb;
  ASSERT_NO_FATAL_FAILURE(sb.initRepo());
  sb.write("a.txt", "a");
  const std::string c1 = sb.addAndCommit({"a.txt"}, "first");

  RunResult again = sb.ark({"init"});
  EXPECT_EQ(again.exitCode, 0) << again;
  EXPECT_EQ(sb.headCommit(), c1);
  EXPECT_TRUE(sb.index().count("a.txt"));
}

TEST(Init, CommandsOutsideARepositoryFail) {
  Sandbox sb; // repo/ exists but was never initialised
  for (const auto &args : std::vector<std::vector<std::string>>{
           {"status"}, {"add", "x"}, {"commit"}, {"log"}, {"branch", "b"},
           {"switch", "b"}, {"write-tree"}}) {
    SCOPED_TRACE(args[0]);
    RunResult r = sb.ark(args);
    EXPECT_NE(r.exitCode, 0) << r;
    EXPECT_LT(r.exitCode, 128) << "crashed instead of failing cleanly\n" << r;
  }
  EXPECT_FALSE(fs::exists(sb.repo() / ".ark"));
}

TEST(Init, UnknownCommandFails) {
  Sandbox sb;
  ASSERT_NO_FATAL_FAILURE(sb.initRepo());
  EXPECT_NE(sb.ark({"frobnicate"}).exitCode, 0);
  EXPECT_NE(sb.ark({}).exitCode, 0);
}

// tmp/bugs/07
TEST(Init, DoesNotCreateLogFilesInTheWorkingDirectory) {
  Sandbox sb;
  sb.ark({"status"}); // outside any repository
  EXPECT_FALSE(fs::exists(sb.repo() / ".ark.log"));

  ASSERT_NO_FATAL_FAILURE(sb.initRepo());
  fs::create_directories(sb.repo() / "sub");
  sb.ark({"status"});
  sb.ark({"status"}, sb.repo() / "sub");
  EXPECT_FALSE(fs::exists(sb.repo() / ".ark.log"));
  EXPECT_FALSE(fs::exists(sb.repo() / "sub/.ark.log"));
}
