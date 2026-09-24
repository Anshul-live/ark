#include "support/ark_test.h"

using namespace arktest;

namespace {

class Commit : public ::testing::Test {
protected:
  void SetUp() override { ASSERT_NO_FATAL_FAILURE(sb.initRepo()); }
  Sandbox sb;
};

} // namespace

// tmp/bugs/25 (root commit must not list a parent)
TEST_F(Commit, RootCommitSnapshotsTheIndexAndHasNoParent) {
  sb.write("a.txt", "A");
  sb.write("d/b.txt", "B");
  const std::string c1 = sb.addAndCommit({"a.txt", "d"}, "initial");

  EXPECT_EQ(sb.branchCommit("main"), c1);
  CommitInfo info = sb.commitInfo(c1);
  EXPECT_TRUE(info.parents.empty()) << "parents[0]=" << info.parents[0];
  EXPECT_EQ(sb.commitFiles(c1), (FileMap{{"a.txt", {blobHash("A"), "100644"}},
                                         {"d/b.txt", {blobHash("B"), "100644"}}}));
}

TEST_F(Commit, EachCommitPointsToThePreviousOne) {
  sb.write("f", "1");
  const std::string c1 = sb.addAndCommit({"f"}, "one");
  sb.write("f", "2");
  const std::string c2 = sb.addAndCommit({"f"}, "two");
  sb.write("f", "3");
  const std::string c3 = sb.addAndCommit({"f"}, "three");

  EXPECT_EQ(sb.commitInfo(c2).parents, std::vector<std::string>{c1});
  EXPECT_EQ(sb.commitInfo(c3).parents, std::vector<std::string>{c2});
  EXPECT_EQ(sb.commitFiles(c3).at("f").first, blobHash("3"));
}

TEST_F(Commit, StoresMessageAndCommitter) {
  sb.write("f", "1");
  const std::string c = sb.addAndCommit({"f"}, "Add the f file");
  CommitInfo info = sb.commitInfo(c);
  EXPECT_EQ(info.message, "Add the f file");
  EXPECT_NE(info.committer.find("Test User <test@example.com>"), std::string::npos)
      << info.committer;
}

// tmp/bugs/22: without a blank line between headers and message, a message
// that looks like a header is misread.
TEST_F(Commit, MessageThatLooksLikeAHeaderIsNotParsedAsOne) {
  sb.write("f", "1");
  const std::string c1 = sb.addAndCommit({"f"}, "first");
  sb.write("f", "2");
  const std::string c2 = sb.addAndCommit({"f"}, "parent " + std::string(64, 'e'));
  EXPECT_EQ(sb.commitInfo(c2).parents, std::vector<std::string>{c1});

  // Ark's own reader must agree: log should walk c2 -> c1, not to eeee...
  RunResult log = sb.ark({"log"});
  EXPECT_EQ(log.exitCode, 0) << log;
  EXPECT_EQ(log.all().find(std::string(64, 'e')), std::string::npos) << log;
}

TEST_F(Commit, OnlyTheCurrentBranchMoves) {
  sb.write("f", "1");
  const std::string c1 = sb.addAndCommit({"f"}, "one");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  sb.write("f", "2");
  sb.addAndCommit({"f"}, "two");
  EXPECT_EQ(sb.branchCommit("feature"), c1);
}

TEST_F(Commit, EmptyMessageAbortsWithoutMovingHead) {
  sb.write("f", "1");
  ASSERT_EQ(sb.ark({"add", "f"}).exitCode, 0);
  RunResult r = sb.commit("");
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headCommit(), "");
}

// tmp/bugs/22
TEST_F(Commit, WhitespaceOnlyMessageAborts) {
  sb.write("f", "1");
  ASSERT_EQ(sb.ark({"add", "f"}).exitCode, 0);
  RunResult r = sb.commit("   \n\t\n");
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headCommit(), "");
}

// tmp/bugs/12
TEST_F(Commit, FailsWithoutIdentityAndLeavesHeadUnchanged) {
  writeFile(sb.repo() / ".ark/config", "");
  sb.write("f", "1");
  ASSERT_EQ(sb.ark({"add", "f"}).exitCode, 0);
  RunResult r = sb.commit("msg");
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headCommit(), "");
}

// tmp/bugs/13
TEST_F(Commit, UsesGlobalIdentityFromHome) {
  writeFile(sb.repo() / ".ark/config", "");
  writeFile(sb.home() / ".arkconfig", "[user]\nname = Global Person\nemail = g@example.com\n");
  sb.write("f", "1");
  ASSERT_EQ(sb.ark({"add", "f"}).exitCode, 0);
  RunResult r = sb.commit("msg");
  ASSERT_EQ(r.exitCode, 0) << r;
  ASSERT_NE(sb.headCommit(), "");
  EXPECT_NE(sb.commitInfo(sb.headCommit()).committer.find("Global Person"), std::string::npos);
}

// tmp/bugs/16
TEST_F(Commit, NothingToCommitIsRefused) {
  sb.write("f", "1");
  const std::string c1 = sb.addAndCommit({"f"}, "one");
  RunResult r = sb.commit("again");
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headCommit(), c1);
}

// tmp/bugs/16
TEST_F(Commit, NothingStagedInFreshRepoIsRefused) {
  RunResult r = sb.commit("empty");
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headCommit(), "");
}
