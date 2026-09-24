#include "support/ark_test.h"

#include <unistd.h>

using namespace arktest;

namespace {

class Add : public ::testing::Test {
protected:
  void SetUp() override { ASSERT_NO_FATAL_FAILURE(sb.initRepo()); }
  Sandbox sb;
};

} // namespace

TEST_F(Add, StagesFileWithCorrectHashModeAndStoredObject) {
  sb.write("a.txt", "alpha\n");
  RunResult r = sb.ark({"add", "a.txt"});
  ASSERT_EQ(r.exitCode, 0) << r;

  FileMap idx = sb.index();
  ASSERT_TRUE(idx.count("a.txt"));
  EXPECT_EQ(idx["a.txt"], std::make_pair(blobHash("alpha\n"), std::string("100644")));

  auto obj = sb.object(blobHash("alpha\n"));
  ASSERT_TRUE(obj) << "blob was not written to the object store";
  EXPECT_EQ(obj->type, "blob");
  EXPECT_EQ(obj->body, "alpha\n");
}

TEST_F(Add, StagesDirectoriesRecursivelyWithRepoRelativePaths) {
  sb.write("src/main.cpp", "int main(){}");
  sb.write("src/util/a.h", "a");
  sb.write("src/util/deep/b.h", "b");
  sb.write("other.txt", "not added");
  ASSERT_EQ(sb.ark({"add", "src"}).exitCode, 0);

  FileMap idx = sb.index();
  EXPECT_EQ(idx.size(), 3u);
  EXPECT_TRUE(idx.count("src/main.cpp"));
  EXPECT_TRUE(idx.count("src/util/a.h"));
  EXPECT_TRUE(idx.count("src/util/deep/b.h"));
}

TEST_F(Add, PathsAreRelativeToTheRepoRootWhenRunFromSubdirectory) {
  sb.write("d/x.txt", "x");
  RunResult r = sb.ark({"add", "x.txt"}, sb.repo() / "d");
  ASSERT_EQ(r.exitCode, 0) << r;
  EXPECT_TRUE(sb.index().count("d/x.txt"));
}

TEST_F(Add, AddingSameFileAgainIsIdempotent) {
  sb.write("a", "a");
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  FileMap first = sb.index();
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  EXPECT_EQ(sb.index(), first);
}

TEST_F(Add, ReaddingAModifiedFileUpdatesItsHash) {
  sb.write("a", "v1");
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  sb.write("a", "v2");
  ASSERT_EQ(sb.ark({"add", "a"}).exitCode, 0);
  EXPECT_EQ(sb.index()["a"].first, blobHash("v2"));
}

TEST_F(Add, RecordsExecutableMode) {
  sb.write("run.sh", "#!/bin/sh\n");
  setExecutable(sb.repo() / "run.sh", true);
  ASSERT_EQ(sb.ark({"add", "run.sh"}).exitCode, 0);
  EXPECT_EQ(sb.index()["run.sh"].second, "100755");
}

TEST_F(Add, RespectsArkignoreForFilesAndDirectories) {
  sb.write(".arkignore", "# comment\n\nsecret.txt\nbuild/\n");
  sb.write("secret.txt", "s");
  sb.write("build/out.o", "o");
  sb.write("keep.txt", "k");
  ASSERT_EQ(sb.ark({"add", "."}).exitCode, 0);

  FileMap idx = sb.index();
  EXPECT_TRUE(idx.count("keep.txt"));
  EXPECT_FALSE(idx.count("secret.txt"));
  EXPECT_FALSE(idx.count("build/out.o"));
}

TEST_F(Add, ExplicitlyNamedIgnoredFileIsNotStaged) {
  sb.write(".arkignore", "secret.txt\n");
  sb.write("secret.txt", "s");
  sb.ark({"add", "secret.txt"});
  EXPECT_FALSE(sb.index().count("secret.txt"));
}

// tmp/bugs/06
TEST_F(Add, DotNeverStagesRepositoryMetadata) {
  sb.write("a.txt", "a");
  sb.write("sub/b.txt", "b");
  ASSERT_EQ(sb.ark({"add", "."}).exitCode, 0);

  for (const auto &[path, entry] : sb.index()) {
    EXPECT_NE(path.rfind(".ark/", 0), 0u) << "staged metadata: " << path;
    EXPECT_NE(fs::path(path).filename(), ".ark.log") << "staged log: " << path;
  }
  EXPECT_TRUE(sb.index().count("a.txt"));
  EXPECT_TRUE(sb.index().count("sub/b.txt"));
}

// tmp/bugs/09
TEST_F(Add, RejectsPathsOutsideTheRepository) {
  writeFile(sb.root() / "outside.txt", "o");
  RunResult r = sb.ark({"add", (sb.root() / "outside.txt").string()});
  EXPECT_NE(r.exitCode, 0) << r;
  for (const auto &[path, entry] : sb.index()) {
    EXPECT_NE(path.rfind("..", 0), 0u) << "staged outside path: " << path;
  }
}

// tmp/bugs/10
TEST_F(Add, UnreadableFileFailsCleanly) {
  if (geteuid() == 0) {
    GTEST_SKIP() << "root can read any file";
  }
  sb.write("ok.txt", "ok");
  ASSERT_EQ(sb.ark({"add", "ok.txt"}).exitCode, 0);
  sb.write("secret", "s");
  fs::permissions(sb.repo() / "secret", fs::perms::none);

  RunResult r = sb.ark({"add", "secret"});
  EXPECT_NE(r.exitCode, 0) << r;
  EXPECT_LT(r.exitCode, 128) << r;
  EXPECT_EQ(r.all().find("basic_string"), std::string::npos) << r;
  EXPECT_EQ(sb.index().size(), 1u) << "index must be unchanged";
}

// tmp/bugs/14
TEST_F(Add, StagesDeletionOfATrackedFile) {
  sb.write("a", "a");
  sb.write("b", "b");
  sb.addAndCommit({"a", "b"}, "both");
  sb.remove("b");

  RunResult r = sb.ark({"add", "b"});
  EXPECT_EQ(r.exitCode, 0) << r;
  EXPECT_FALSE(sb.index().count("b"));
  EXPECT_TRUE(sb.index().count("a"));
}

// tmp/bugs/14
TEST_F(Add, DotStagesDeletions) {
  sb.write("a", "a");
  sb.write("d/b", "b");
  sb.addAndCommit({"a", "d"}, "both");
  sb.remove("d");

  ASSERT_EQ(sb.ark({"add", "."}).exitCode, 0);
  EXPECT_FALSE(sb.index().count("d/b"));
  EXPECT_TRUE(sb.index().count("a"));
}

// tmp/bugs/05
TEST_F(Add, PathsWithSpacesAreStagedExactly) {
  sb.write("my file.txt", "1");
  sb.write("dir with space/x  y.txt", "2");
  sb.write("plain.txt", "3");
  ASSERT_EQ(sb.ark({"add", "my file.txt", "dir with space", "plain.txt"}).exitCode, 0);

  EXPECT_EQ(sb.index(), (FileMap{{"my file.txt", {blobHash("1"), "100644"}},
                                 {"dir with space/x  y.txt", {blobHash("2"), "100644"}},
                                 {"plain.txt", {blobHash("3"), "100644"}}}));
}
