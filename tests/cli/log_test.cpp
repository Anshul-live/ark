#include "support/ark_test.h"

#include <algorithm>

using namespace arktest;

namespace {

class Log : public ::testing::Test {
protected:
  void SetUp() override { ASSERT_NO_FATAL_FAILURE(sb.initRepo()); }

  std::vector<std::string> commits() {
    for (int i = 1; i <= 3; i++) {
      sb.write("f", std::to_string(i));
      history.insert(history.begin(), sb.addAndCommit({"f"}, "c" + std::to_string(i)));
    }
    return history;
  }

  Sandbox sb;
  std::vector<std::string> history; // newest first
};

} // namespace

TEST_F(Log, EmptyRepositorySucceedsAndListsNothing) {
  RunResult r = sb.ark({"log"});
  EXPECT_EQ(r.exitCode, 0) << r;
  EXPECT_TRUE(hashesIn(r.out).empty()) << r;
}

TEST_F(Log, ListsEveryCommitOnceNewestFirst) {
  commits();
  RunResult r = sb.ark({"log"});
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_EQ(hashesIn(r.out), history) << r;
}

TEST_F(Log, ShowsTheCurrentBranchOnly) {
  commits();
  ASSERT_EQ(sb.ark({"branch", "old"}).exitCode, 0);
  sb.write("f", "4");
  const std::string c4 = sb.addAndCommit({"f"}, "c4");
  ASSERT_EQ(sb.ark({"switch", "old"}).exitCode, 0);

  auto shown = hashesIn(sb.ark({"log"}).out);
  EXPECT_EQ(shown, history);
  EXPECT_EQ(std::count(shown.begin(), shown.end(), c4), 0);
}

// tmp/bugs/21 (also needs tmp/bugs/11 for commit-tree parents)
TEST_F(Log, MergeHistoryListsEachCommitOnce) {
  commits();
  const std::string tree = sb.commitInfo(history[0]).tree;
  // merge commit whose two parents share every ancestor
  RunResult m = sb.ark({"commit-tree", tree, history[0], history[1]});
  ASSERT_EQ(m.exitCode, 0) << m;
  const std::string merge = hashesIn(m.out).back();
  ASSERT_EQ(sb.ark({"update-ref", "refs/heads/main", merge}).exitCode, 0);

  auto shown = hashesIn(sb.ark({"log"}).out);
  std::set<std::string> unique(shown.begin(), shown.end());
  EXPECT_EQ(unique.size(), shown.size()) << "duplicates in log";
  EXPECT_EQ(unique.size(), 4u);
}

// tmp/bugs/21
TEST_F(Log, WorksOnDetachedHead) {
  commits();
  writeFile(sb.repo() / ".ark/HEAD", history[1]);
  RunResult r = sb.ark({"log"});
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_EQ(hashesIn(r.out), (std::vector<std::string>{history[1], history[2]}));
}

// tmp/bugs/20
TEST_F(Log, DanglingRefFailsCleanly) {
  commits();
  writeFile(sb.repo() / ".ark/refs/heads/main", std::string(64, 'd'));
  RunResult r = sb.ark({"log"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_LT(r.exitCode, 128) << r;
  EXPECT_EQ(r.all().find("basic_string"), std::string::npos) << r;
}
