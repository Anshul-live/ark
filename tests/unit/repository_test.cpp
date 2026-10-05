#include "support/ark_test.h"

#include <ark/core/exceptions.h>
#include <ark/cli/commands.h>
#include <ark/core/file_mode.h>
#include <ark/repo/ignore_rules.h>
#include <ark/repo/repository.h>
#include <ark/repo/working_tree.h>
#include <ark/util/fs.h>
#include <ark/util/time.h>

#include <regex>

using namespace arktest;

// Note: IgnoreRules is loaded per command, so
// ignore rules are tested through the CLI (one process per command) instead.

namespace {

class RepositoryTest : public ::testing::Test {
protected:
  void SetUp() override {
    cwd_ = std::make_unique<CwdGuard>(tmp_.path());
    ASSERT_EQ(cmd_init({}), 0);
  }
  void TearDown() override { cwd_.reset(); }

  TempDir tmp_;
  std::unique_ptr<CwdGuard> cwd_;
};

} // namespace

TEST_F(RepositoryTest, PathsAreRootedAtTheRepository) {
  Repository repo;
  const std::string root = tmp_.path().string();
  EXPECT_EQ(repo.root(), root);
  EXPECT_EQ(repo.arkPath(), root + "/.ark");
  EXPECT_EQ(repo.objectsDir(), root + "/.ark/objects");
  EXPECT_EQ(repo.indexPath(), root + "/.ark/index");
  EXPECT_EQ(repo.headPath(), root + "/.ark/HEAD");
  EXPECT_EQ(repo.refsDir(), root + "/.ark/refs");
  EXPECT_EQ(repo.configPath(), root + "/.ark/config");
  EXPECT_TRUE(repo.isValid());
}

TEST_F(RepositoryTest, FindsRootFromNestedSubdirectory) {
  fs::create_directories("a/b/c");
  CwdGuard inner(tmp_.path() / "a/b/c");
  EXPECT_EQ(Repository().root(), tmp_.path().string());
}

TEST(RepositoryOutside, ThrowsNotARepository) {
  TempDir outside;
  CwdGuard cwd(outside.path());
  Repository repo;
  EXPECT_THROW(repo.root(), NotARepositoryException);
  EXPECT_FALSE(repo.isValid());
}

TEST_F(RepositoryTest, ModeReflectsFileType) {
  writeFile("plain", "x");
  writeFile("script", "#!/bin/sh\n");
  setExecutable("script", true);
  fs::create_directory("dir");
  fs::create_symlink("plain", "link");

  EXPECT_EQ(ark::toOctalString(*ark::detectFileMode("plain")), "100644");
  EXPECT_EQ(ark::toOctalString(*ark::detectFileMode("script")), "100755");
  EXPECT_EQ(ark::toOctalString(*ark::detectFileMode("dir")), "040000");
  EXPECT_EQ(ark::toOctalString(*ark::detectFileMode("link")), "120000");
}

TEST_F(RepositoryTest, NormalizePath) {
  EXPECT_EQ(ark::normalizeGenericPath("a/./b/../c"), "a/c");
  EXPECT_EQ(ark::normalizeGenericPath("a//b"), "a/b");
}

TEST_F(RepositoryTest, TimezoneOffsetFormat) {
  EXPECT_TRUE(std::regex_match(ark::formatLocalUtcOffset(),
                               std::regex("^[+-][0-9]{4}$")));
}

// tmp/bugs/06
TEST_F(RepositoryTest, WorkingDirectoryScanNeverIncludesArkMetadata) {
  writeFile("tracked.txt", "x");
  writeFile("sub/nested.txt", "y");
  Repository repo;
  ark::IgnoreRules ignore = ark::IgnoreRules::load(repo);
  auto files = ark::WorkingTree(repo, ignore).scan();
  for (const auto &[path, entry] : files) {
    EXPECT_NE(path.str().rfind(".ark/", 0), 0u) << path;
  }
  EXPECT_TRUE(files.count(ark::RepoPath::fromRelative("tracked.txt")));
  EXPECT_TRUE(files.count(ark::RepoPath::fromRelative("sub/nested.txt")));
}
