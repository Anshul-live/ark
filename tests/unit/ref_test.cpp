#include "support/ark_test.h"

#include <ark/core/exceptions.h>
#include <ark/cli/commands.h>
#include <ark/repo/ref_store.h>

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

  static ark::ObjectHash hash(char c) {
    return ark::ObjectHash::parse(std::string(64, c)).value();
  }
  static ark::BranchName branch(const std::string &name) {
    return ark::BranchName::unchecked(name);
  }
  static std::string hex(const std::optional<ark::ObjectHash> &h) {
    return h ? h->hex() : "";
  }
  std::string head() const { return trim(readFile(tmp_.path() / ".ark/HEAD")); }

  TempDir tmp_;
  std::unique_ptr<CwdGuard> cwd_;
  RefStore ref_;
};

} // namespace

TEST_F(RefTest, FreshRepositoryIsOnMain) {
  ASSERT_TRUE(ref_.currentBranch());
  EXPECT_EQ(ref_.currentBranch()->str(), "main");
  EXPECT_EQ(std::get<ark::RefName>(ref_.readHead()).str(), "refs/heads/main");
  EXPECT_FALSE(ref_.isHeadDetached());
}

TEST_F(RefTest, CreateBranchWritesRefAndIsListed) {
  ref_.createBranch(branch("feature"), hash('a'));
  EXPECT_TRUE(ref_.branchExists(branch("feature")));
  EXPECT_EQ(hex(ref_.branchTarget(branch("feature"))), hash('a').hex());

  auto branches = ref_.listBranches();
  std::vector<std::string> names;
  for (const auto &b : branches)
    names.push_back(b.str());
  std::sort(names.begin(), names.end());
  EXPECT_EQ(names, (std::vector<std::string>{"feature", "main"}));
}

TEST_F(RefTest, CreatingExistingBranchThrows) {
  ref_.createBranch(branch("feature"), hash('a'));
  EXPECT_THROW(ref_.createBranch(branch("feature"), hash('b')), BranchExistsException);
  EXPECT_EQ(hex(ref_.branchTarget(branch("feature"))), hash('a').hex());
}

TEST_F(RefTest, SetHeadToCommitOnBranchMovesTheBranch) {
  ref_.moveHeadTo(hash('c'));
  EXPECT_EQ(hex(ref_.branchTarget(branch("main"))), hash('c').hex());
  EXPECT_EQ(hex(ref_.resolveHead()), hash('c').hex());
  EXPECT_EQ(head(), "ref: refs/heads/main");
}

TEST_F(RefTest, SwitchingHeadToBranch) {
  ref_.createBranch(branch("feature"), hash('f'));
  ref_.attachHeadTo(branch("feature"));
  EXPECT_EQ(head(), "ref: refs/heads/feature");
  ASSERT_TRUE(ref_.currentBranch());
  EXPECT_EQ(ref_.currentBranch()->str(), "feature");
  EXPECT_EQ(hex(ref_.resolveHead()), hash('f').hex());
}

TEST_F(RefTest, SwitchingToMissingBranchThrowsAndKeepsHead) {
  EXPECT_THROW(ref_.attachHeadTo(branch("nope")), BranchNotFoundException);
  EXPECT_EQ(head(), "ref: refs/heads/main");
}

TEST_F(RefTest, DetachedHead) {
  const std::string mainBefore = hex(ref_.branchTarget(branch("main")));
  writeFile(tmp_.path() / ".ark/HEAD", hash('d').hex());
  EXPECT_TRUE(ref_.isHeadDetached());
  EXPECT_EQ(hex(ref_.resolveHead()), hash('d').hex());
  EXPECT_FALSE(ref_.currentBranch());

  ref_.moveHeadTo(hash('e'));
  EXPECT_EQ(head(), hash('e').hex());
  EXPECT_EQ(hex(ref_.branchTarget(branch("main"))), mainBefore) << "branch must not move";
}

TEST_F(RefTest, DeleteBranch) {
  ref_.createBranch(branch("tmp"), hash('b'));
  ref_.deleteBranch(branch("tmp"));
  EXPECT_FALSE(ref_.branchExists(branch("tmp")));
  EXPECT_THROW(ref_.deleteBranch(branch("tmp")), BranchNotFoundException);
}

// tmp/bugs/08
TEST_F(RefTest, RejectsBranchNamesThatEscapeRefs) {
  for (const std::string bad : {"../../escaped", "/abs", "a..b", "has space", "-dash", "x.lock"}) {
    SCOPED_TRACE(bad);
    EXPECT_ANY_THROW(ref_.createBranch(branch(bad), hash('a')));
  }
  EXPECT_FALSE(fs::exists(tmp_.path() / "escaped"));
  EXPECT_FALSE(fs::exists(tmp_.path().parent_path() / "escaped"));
}
