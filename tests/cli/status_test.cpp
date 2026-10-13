#include "support/ark_test.h"

using namespace arktest;

namespace {

class StatusTest : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(sb.initRepo());
    // Keep these tests independent of tmp/bugs/06 (.ark leaking into status);
    // that bug has its own test below which does not use this file.
    sb.write(".arkignore", ".ark/\n.ark.log\n");
  }

  // Commits a small tree with top-level and nested files, plus .arkignore.
  std::string commitBaseline() {
    sb.write("a.txt", "A");
    sb.write("b.txt", "B");
    sb.write("d/c.txt", "C");
    sb.write("d/e/f.txt", "F");
    return sb.addAndCommit({".arkignore", "a.txt", "b.txt", "d"}, "baseline");
  }

  Sandbox sb;
};

} // namespace

TEST_F(StatusTest, ShowsUntrackedFiles) {
  sb.write("new.txt", "n");
  Status st = sb.status();
  EXPECT_TRUE(st.untracked.count("new.txt")) << st.raw;
  EXPECT_FALSE(st.staged.count("new.txt")) << st.raw;
}

TEST_F(StatusTest, ShowsStagedNewFiles) {
  sb.write("new.txt", "n");
  ASSERT_EQ(sb.ark({"add", "new.txt"}).exitCode, 0);
  Status st = sb.status();
  EXPECT_TRUE(st.staged.count("new.txt")) << st.raw;
  EXPECT_FALSE(st.untracked.count("new.txt")) << st.raw;
}

// tmp/bugs/01, tmp/bugs/02
TEST_F(StatusTest, CleanRightAfterCommit) {
  commitBaseline();
  Status st = sb.status();
  EXPECT_TRUE(st.clean()) << st.raw;
}

// tmp/bugs/01, tmp/bugs/02
TEST_F(StatusTest, ShowsUnstagedModificationOfTrackedFile) {
  commitBaseline();
  sb.write("d/e/f.txt", "changed");
  Status st = sb.status();
  EXPECT_EQ(st.modified, std::set<std::string>{"d/e/f.txt"}) << st.raw;
  EXPECT_TRUE(st.staged.empty()) << st.raw;
  EXPECT_TRUE(st.untracked.empty()) << st.raw;
}

// tmp/bugs/01, tmp/bugs/02
TEST_F(StatusTest, StagedModificationIsReportedOnceAsStaged) {
  commitBaseline();
  sb.write("a.txt", "changed");
  ASSERT_EQ(sb.ark({"add", "a.txt"}).exitCode, 0);
  Status st = sb.status();
  EXPECT_EQ(st.staged, std::set<std::string>{"a.txt"}) << st.raw;
  EXPECT_TRUE(st.modified.empty()) << st.raw;
}

// tmp/bugs/14
TEST_F(StatusTest, ShowsDeletedFiles) {
  commitBaseline();
  sb.remove("b.txt");
  Status st = sb.status();
  EXPECT_TRUE(st.deleted.count("b.txt")) << st.raw;
}

// tmp/bugs/15
TEST_F(StatusTest, SameResultFromASubdirectory) {
  commitBaseline();
  sb.write("d/c.txt", "changed");
  sb.write("new.txt", "n");
  Status fromRoot = sb.status();
  Status fromSub = sb.status(sb.repo() / "d");
  EXPECT_EQ(fromSub.staged, fromRoot.staged) << fromSub.raw;
  EXPECT_EQ(fromSub.modified, fromRoot.modified) << fromSub.raw;
  EXPECT_EQ(fromSub.untracked, fromRoot.untracked) << fromSub.raw;
  EXPECT_TRUE(fromSub.modified.count("d/c.txt")) << fromSub.raw;
}

TEST_F(StatusTest, IgnoredFilesAreNotUntracked) {
  sb.write(".arkignore", ".ark/\n.ark.log\nsecret.txt\nbuild/\n");
  sb.write("secret.txt", "s");
  sb.write("build/o", "o");
  Status st = sb.status();
  EXPECT_FALSE(st.untracked.count("secret.txt")) << st.raw;
  EXPECT_FALSE(st.untracked.count("build/o")) << st.raw;
}

// tmp/bugs/06
TEST(StatusMetadata, NeverListsRepositoryMetadata) {
  Sandbox sb;
  ASSERT_NO_FATAL_FAILURE(sb.initRepo());
  sb.write("a.txt", "a");
  sb.addAndCommit({"a.txt"}, "one");
  Status st = sb.status();
  for (const auto *set : {&st.staged, &st.modified, &st.untracked, &st.deleted}) {
    for (const auto &path : *set) {
      EXPECT_NE(path.rfind(".ark", 0), 0u) << path << "\n" << st.raw;
    }
  }
}
