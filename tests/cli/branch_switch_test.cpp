#include "support/ark_test.h"

using namespace arktest;

namespace {

class Branching : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(sb.initRepo());
    sb.write(".arkignore", ".ark/\n.ark.log\n");
  }

  // feature: {a.txt=one, d/nested.txt=n1}
  // main:    {a.txt=two, d/nested.txt=n2, only-main.txt, bin/blob.dat}
  void buildTwoBranches() {
    sb.write("a.txt", "one\n");
    sb.write("d/nested.txt", "n1\n");
    base = sb.addAndCommit({".arkignore", "a.txt", "d"}, "base");
    ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
    featureSnap = sb.worktree();

    sb.write("a.txt", "two\n");
    sb.write("d/nested.txt", "n2\n");
    sb.write("only-main.txt", "main only\n");
    sb.write("bin/blob.dat", std::string("\0\x01\x02\xff binary\0", 12));
    tip = sb.addAndCommit({"a.txt", "d", "only-main.txt", "bin"}, "main work");
    mainSnap = sb.worktree();
  }

  Sandbox sb;
  std::string base, tip;
  Snapshot featureSnap, mainSnap;
};

} // namespace

TEST_F(Branching, BranchRequiresACommit) {
  RunResult r = sb.ark({"branch", "feature"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.branchCommit("feature"), "");
}

TEST_F(Branching, NewBranchPointsAtHead) {
  sb.write("f", "1");
  const std::string c = sb.addAndCommit({"f"}, "one");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  EXPECT_EQ(sb.branchCommit("feature"), c);
  EXPECT_EQ(sb.headFile(), "ref: refs/heads/main") << "branch must not switch";
}

TEST_F(Branching, ListMarksTheCurrentBranch) {
  sb.write("f", "1");
  sb.addAndCommit({"f"}, "one");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  RunResult r = sb.ark({"branch"});
  ASSERT_EQ(r.exitCode, 0) << r;
  const std::string out = stripAnsi(r.out);
  EXPECT_NE(out.find("* main"), std::string::npos) << out;
  EXPECT_NE(out.find("  feature"), std::string::npos) << out;
}

TEST_F(Branching, DuplicateBranchFailsAndKeepsOriginal) {
  sb.write("f", "1");
  const std::string c1 = sb.addAndCommit({"f"}, "one");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  sb.write("f", "2");
  sb.addAndCommit({"f"}, "two");
  EXPECT_NE(sb.ark({"branch", "feature"}).exitCode, 0);
  EXPECT_EQ(sb.branchCommit("feature"), c1);
}

// tmp/bugs/08
TEST_F(Branching, RejectsBranchNamesThatEscapeRefs) {
  sb.write("f", "1");
  sb.addAndCommit({"f"}, "one");
  RunResult r = sb.ark({"branch", "../../evil"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_FALSE(fs::exists(sb.repo() / ".ark/evil"));
  EXPECT_FALSE(fs::exists(sb.repo() / "evil"));
}

TEST_F(Branching, SwitchToUnknownBranchFails) {
  sb.write("f", "1");
  sb.addAndCommit({"f"}, "one");
  RunResult r = sb.ark({"switch", "nope"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headFile(), "ref: refs/heads/main");
}

TEST_F(Branching, SwitchMovesHead) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  RunResult r = sb.ark({"switch", "feature"});
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_EQ(sb.headFile(), "ref: refs/heads/feature");
  EXPECT_EQ(sb.headCommit(), base);
}

// tmp/bugs/01, tmp/bugs/02
TEST_F(Branching, SwitchReproducesTheTargetSnapshotExactly) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());

  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  Snapshot now = sb.worktree();
  EXPECT_EQ(now, featureSnap) << "expected:\n" << describe(featureSnap) << "actual:\n" << describe(now);

  ASSERT_EQ(sb.ark({"switch", "main"}).exitCode, 0);
  now = sb.worktree();
  EXPECT_EQ(now, mainSnap) << "expected:\n" << describe(mainSnap) << "actual:\n" << describe(now);
}

// tmp/bugs/02
TEST_F(Branching, IndexMatchesTargetCommitAfterSwitch) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  EXPECT_EQ(sb.index(), sb.commitFiles(base));
  ASSERT_EQ(sb.ark({"switch", "main"}).exitCode, 0);
  EXPECT_EQ(sb.index(), sb.commitFiles(tip));
}

// tmp/bugs/01, tmp/bugs/02
TEST_F(Branching, StatusIsCleanAfterSwitch) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  Status st = sb.status();
  EXPECT_TRUE(st.clean()) << st.raw;
}

// tmp/bugs/02: a commit made after switching must contain the right tree.
TEST_F(Branching, CommitAfterSwitchBuildsOnTheTargetTree) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  sb.write("feature.txt", "f");
  const std::string c = sb.addAndCommit({"feature.txt"}, "on feature");

  FileMap expected = sb.commitFiles(base);
  expected["feature.txt"] = {blobHash("f"), "100644"};
  EXPECT_EQ(sb.commitFiles(c), expected);
  EXPECT_EQ(sb.commitInfo(c).parents, std::vector<std::string>{base});
}

TEST_F(Branching, SwitchRestoresExecutableBit) {
  sb.write("f", "1");
  sb.addAndCommit({".arkignore", "f"}, "one");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  sb.write("tool.sh", "#!/bin/sh\n");
  setExecutable(sb.repo() / "tool.sh", true);
  sb.addAndCommit({"tool.sh"}, "add tool");

  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  EXPECT_FALSE(sb.exists("tool.sh"));
  ASSERT_EQ(sb.ark({"switch", "main"}).exitCode, 0);
  ASSERT_TRUE(sb.exists("tool.sh"));
  EXPECT_TRUE(isExecutable(sb.repo() / "tool.sh"));
}

// tmp/bugs/18
TEST_F(Branching, SwitchAppliesModeOnlyChanges) {
  sb.write("tool.sh", "#!/bin/sh\n");
  sb.addAndCommit({".arkignore", "tool.sh"}, "not executable");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  setExecutable(sb.repo() / "tool.sh", true);
  sb.addAndCommit({"tool.sh"}, "make executable");

  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  ASSERT_TRUE(sb.exists("tool.sh"));
  EXPECT_FALSE(isExecutable(sb.repo() / "tool.sh"));
  ASSERT_EQ(sb.ark({"switch", "main"}).exitCode, 0);
  ASSERT_TRUE(sb.exists("tool.sh"));
  EXPECT_TRUE(isExecutable(sb.repo() / "tool.sh"));
}

// tmp/bugs/18
TEST_F(Branching, SwitchHandlesFileReplacedByDirectory) {
  sb.write("thing", "i am a file");
  sb.addAndCommit({".arkignore", "thing"}, "file");
  ASSERT_EQ(sb.ark({"branch", "feature"}).exitCode, 0);
  Snapshot asFile = sb.worktree();

  sb.remove("thing");
  sb.write("thing/inside.txt", "now a dir");
  ASSERT_EQ(sb.ark({"add", "thing"}).exitCode, 0);
  sb.commit("dir");
  Snapshot asDir = sb.worktree();

  ASSERT_EQ(sb.ark({"switch", "feature"}).exitCode, 0);
  EXPECT_EQ(sb.worktree(), asFile) << describe(sb.worktree());
  ASSERT_EQ(sb.ark({"switch", "main"}).exitCode, 0);
  EXPECT_EQ(sb.worktree(), asDir) << describe(sb.worktree());
}

// tmp/bugs/03
TEST_F(Branching, SwitchRefusesToOverwriteUncommittedChanges) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  sb.write("a.txt", "precious work in progress\n"); // differs on both branches

  RunResult r = sb.ark({"switch", "feature"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.read("a.txt"), "precious work in progress\n");
  EXPECT_EQ(sb.headFile(), "ref: refs/heads/main");
}

// tmp/bugs/03
TEST_F(Branching, SwitchNeverDropsStagedChanges) {
  ASSERT_NO_FATAL_FAILURE(buildTwoBranches());
  sb.write("staged-new.txt", "s");
  ASSERT_EQ(sb.ark({"add", "staged-new.txt"}).exitCode, 0);

  RunResult r = sb.ark({"switch", "feature"});
  // Either refusing or carrying the change over is acceptable; losing it is not.
  EXPECT_TRUE(sb.index().count("staged-new.txt")) << r;
  EXPECT_TRUE(sb.exists("staged-new.txt")) << r;
}
