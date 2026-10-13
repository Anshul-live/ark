#include "support/ark_test.h"

#include <exceptions.h>
#include <init.h>
#include <ref.h>

#include <algorithm>

using namespace arktest;

namespace {

class RefTest : public ::testing::Test {
protected:
  void SetUp() override {
    cwd_ = std::make_unique<CwdGuard>(tmp_.path());
    ASSERT_EQ(cmd_init({}), 0);
  }
  void TearDown() override { cwd_.reset(); }

  static std::string hash(char c) { return std::string(64, c); }
  std::string head() const { return trim(readFile(tmp_.path() / ".ark/HEAD")); }

  TempDir tmp_;
  std::unique_ptr<CwdGuard> cwd_;
  Ref ref_;
};

} // namespace

TEST_F(RefTest, FreshRepositoryIsOnMain) {
  EXPECT_EQ(ref_.getCurrentBranchName(), "main");
  EXPECT_EQ(ref_.getHeadBranch(), "refs/heads/main");
  EXPECT_FALSE(ref_.isDetached());
}

TEST_F(RefTest, CreateBranchWritesRefAndIsListed) {
  ref_.createBranch("feature", hash('a'));
  EXPECT_TRUE(ref_.branchExists("feature"));
  EXPECT_EQ(ref_.getBranchHash("feature"), hash('a'));

  auto branches = ref_.listBranches();
  std::sort(branches.begin(), branches.end());
  EXPECT_EQ(branches, (std::vector<std::string>{"feature", "main"}));
}

TEST_F(RefTest, CreatingExistingBranchThrows) {
  ref_.createBranch("feature", hash('a'));
  EXPECT_THROW(ref_.createBranch("feature", hash('b')), BranchExistsException);
  EXPECT_EQ(ref_.getBranchHash("feature"), hash('a'));
}

TEST_F(RefTest, SetHeadToCommitOnBranchMovesTheBranch) {
  ref_.setHeadToCommit(hash('c'));
  EXPECT_EQ(ref_.getBranchHash("main"), hash('c'));
  EXPECT_EQ(ref_.getHeadCommit(), hash('c'));
  EXPECT_EQ(head(), "ref: refs/heads/main");
}

TEST_F(RefTest, SwitchingHeadToBranch) {
  ref_.createBranch("feature", hash('f'));
  ref_.setHeadToBranch("feature");
  EXPECT_EQ(head(), "ref: refs/heads/feature");
  EXPECT_EQ(ref_.getCurrentBranchName(), "feature");
  EXPECT_EQ(ref_.getHeadCommit(), hash('f'));
}

TEST_F(RefTest, SwitchingToMissingBranchThrowsAndKeepsHead) {
  EXPECT_THROW(ref_.setHeadToBranch("nope"), BranchNotFoundException);
  EXPECT_EQ(head(), "ref: refs/heads/main");
}

TEST_F(RefTest, DetachedHead) {
  const std::string mainBefore = ref_.getBranchHash("main");
  writeFile(tmp_.path() / ".ark/HEAD", hash('d'));
  EXPECT_TRUE(ref_.isDetached());
  EXPECT_EQ(ref_.getHeadCommit(), hash('d'));
  EXPECT_EQ(ref_.getCurrentBranchName(), "");

  ref_.setHeadToCommit(hash('e'));
  EXPECT_EQ(head(), hash('e'));
  EXPECT_EQ(ref_.getBranchHash("main"), mainBefore) << "branch must not move";
}

TEST_F(RefTest, DeleteBranch) {
  ref_.createBranch("tmp", hash('t'));
  ref_.deleteBranch("tmp");
  EXPECT_FALSE(ref_.branchExists("tmp"));
  EXPECT_THROW(ref_.deleteBranch("tmp"), BranchNotFoundException);
}

// tmp/bugs/08
TEST_F(RefTest, RejectsBranchNamesThatEscapeRefs) {
  for (const std::string bad : {"../../escaped", "/abs", "a..b", "has space", "-dash", "x.lock"}) {
    SCOPED_TRACE(bad);
    EXPECT_ANY_THROW(ref_.createBranch(bad, hash('a')));
  }
  EXPECT_FALSE(fs::exists(tmp_.path() / "escaped"));
  EXPECT_FALSE(fs::exists(tmp_.path().parent_path() / "escaped"));
}
