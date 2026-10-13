#include "support/ark_test.h"

using namespace arktest;

namespace {

class Plumbing : public ::testing::Test {
protected:
  void SetUp() override { ASSERT_NO_FATAL_FAILURE(sb.initRepo()); }

  // Last hash printed by a command. Used where the test is about the value,
  // not about stdout being clean (that has its own tests).
  static std::string lastHash(const RunResult &r) {
    auto hs = hashesIn(r.out);
    return hs.empty() ? "" : hs.back();
  }

  Sandbox sb;
};

} // namespace

TEST_F(Plumbing, HashObjectMatchesIndependentSha256) {
  sb.write("f.txt", "hello\n");
  RunResult r = sb.ark({"hash-object", "f.txt"});
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_EQ(lastHash(r), blobHash("hello\n"));
}

TEST_F(Plumbing, HashObjectHashesBinaryAndEmptyFilesExactly) {
  const std::string bytes("\0\xff\x01\r\n\0", 6);
  sb.write("bin", bytes);
  sb.write("empty", "");
  EXPECT_EQ(lastHash(sb.ark({"hash-object", "bin"})), blobHash(bytes));
  EXPECT_EQ(lastHash(sb.ark({"hash-object", "empty"})), blobHash(""));
}

// tmp/bugs/17
TEST_F(Plumbing, HashObjectPrintsOnlyTheHash) {
  sb.write("f.txt", "x");
  RunResult r = sb.ark({"hash-object", "f.txt"});
  EXPECT_EQ(trim(r.out), blobHash("x")) << r;
}

// tmp/bugs/10, tmp/bugs/12
TEST_F(Plumbing, HashObjectOfMissingFileFails) {
  RunResult r = sb.ark({"hash-object", "missing.txt"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_TRUE(hashesIn(r.out).empty()) << r;
}

TEST_F(Plumbing, CatFilePrintsStoredBlobContent) {
  const std::string content = "line one\nline two\n";
  sb.write("f.txt", content);
  ASSERT_EQ(sb.ark({"add", "f.txt"}).exitCode, 0);

  RunResult r = sb.ark({"cat-file", blobHash(content)});
  ASSERT_EQ(r.exitCode, 0) << r;
  // Ark prints one extra trailing newline after the content; allow it but nothing else.
  EXPECT_TRUE(r.out == content || r.out == content + "\n") << r;
}

TEST_F(Plumbing, CatFileOfUnknownObjectFails) {
  RunResult r = sb.ark({"cat-file", std::string(64, 'a')});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_LT(r.exitCode, 128) << r;
}

TEST_F(Plumbing, WriteTreeStoresTheIndexAsNestedTrees) {
  sb.write("a.txt", "A");
  sb.write("d/b.txt", "B");
  sb.write("d/e/c.txt", "C");
  ASSERT_EQ(sb.ark({"add", "a.txt", "d"}).exitCode, 0);

  RunResult r = sb.ark({"write-tree"});
  ASSERT_EQ(r.exitCode, 0) << r;
  const std::string tree = lastHash(r);
  ASSERT_TRUE(isHash(tree)) << r;

  auto rootObj = sb.object(tree);
  ASSERT_TRUE(rootObj);
  EXPECT_EQ(rootObj->type, "tree");
  std::map<std::string, std::string> rootTypes;
  for (const auto &e : parseTree(rootObj->body)) {
    rootTypes[e.name] = e.type;
  }
  EXPECT_EQ(rootTypes, (std::map<std::string, std::string>{{"a.txt", "blob"}, {"d", "tree"}}));

  EXPECT_EQ(flattenTree(sb.repo(), tree),
            (FileMap{{"a.txt", {blobHash("A"), "100644"}},
                     {"d/b.txt", {blobHash("B"), "100644"}},
                     {"d/e/c.txt", {blobHash("C"), "100644"}}}));
}

// tmp/bugs/17
TEST_F(Plumbing, WriteTreePrintsOnlyTheHash) {
  sb.write("a", "a");
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  RunResult r = sb.ark({"write-tree"});
  EXPECT_TRUE(isHash(trim(r.out))) << r;
}

// tmp/bugs/04
TEST(PlumbingDeterminism, TreeHashDoesNotDependOnStagingOrder) {
  std::vector<std::string> names;
  for (char c = 'a'; c <= 'p'; c++) {
    names.push_back(std::string(1, c));
  }
  auto treeFor = [&](const std::vector<std::string> &order) {
    Sandbox sb;
    sb.initRepo();
    for (const auto &n : names) {
      sb.write(n, "content " + n);
    }
    std::vector<std::string> args{"add"};
    args.insert(args.end(), order.begin(), order.end());
    EXPECT_EQ(sb.ark(args).exitCode, 0);
    return hashesIn(sb.ark({"write-tree"}).out).back();
  };
  std::vector<std::string> reversed(names.rbegin(), names.rend());
  EXPECT_EQ(treeFor(names), treeFor(reversed));
}

// tmp/bugs/11
TEST_F(Plumbing, CommitTreeRecordsTheGivenParent) {
  sb.write("a", "a");
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  const std::string tree = lastHash(sb.ark({"write-tree"}));

  RunResult r1 = sb.ark({"commit-tree", tree});
  ASSERT_EQ(r1.exitCode, 0) << r1;
  const std::string c1 = lastHash(r1);
  ASSERT_TRUE(sb.object(c1)) << r1;
  EXPECT_EQ(sb.commitInfo(c1).tree, tree);

  RunResult r2 = sb.ark({"commit-tree", tree, c1});
  ASSERT_EQ(r2.exitCode, 0) << r2;
  const std::string c2 = lastHash(r2);
  EXPECT_EQ(sb.commitInfo(c2).parents, std::vector<std::string>{c1});
}

TEST_F(Plumbing, UpdateRefMovesABranch) {
  sb.write("a", "a");
  const std::string c1 = sb.addAndCommit({"a"}, "one");
  sb.write("a", "b");
  const std::string c2 = sb.addAndCommit({"a"}, "two");

  RunResult r = sb.ark({"update-ref", "refs/heads/main", c1});
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_EQ(sb.branchCommit("main"), c1);
  EXPECT_NE(c1, c2);
}

// tmp/bugs/08
TEST_F(Plumbing, UpdateRefRejectsPathsOutsideRefs) {
  sb.write("a", "a");
  const std::string c1 = sb.addAndCommit({"a"}, "one");
  RunResult r = sb.ark({"update-ref", "../../escaped", c1});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_FALSE(fs::exists(sb.root() / "escaped"));
  EXPECT_FALSE(fs::exists(sb.repo() / "escaped"));
}

// tmp/bugs/08
TEST_F(Plumbing, UpdateRefRejectsHashesThatAreNotCommits) {
  sb.write("a", "a");
  const std::string c1 = sb.addAndCommit({"a"}, "one");
  RunResult r = sb.ark({"update-ref", "refs/heads/main", "not-a-hash"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_EQ(sb.branchCommit("main"), c1);
}
