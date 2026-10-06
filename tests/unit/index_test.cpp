#include "support/ark_test.h"

#include <ark/repo/index.h>
#include <ark/repo/index_file.h>
#include <sstream>
#include <ark/cli/commands.h>

using namespace arktest;

namespace {

class IndexTest : public ::testing::Test {
protected:
  void SetUp() override {
    cwd_ = std::make_unique<CwdGuard>(tmp_.path());
    ASSERT_EQ(cmd_init({}), 0);
  }
  void TearDown() override { cwd_.reset(); }

  static ark::ObjectHash fakeHash(char c) {
    return ark::ObjectHash::parse(std::string(64, c)).value();
  }
  static ark::FileEntry entry(char c, const std::string &mode) {
    return ark::FileEntry{fakeHash(c), ark::parseFileMode(mode).value()};
  }
  static ark::RepoPath path(const std::string &p) {
    return ark::RepoPath::fromRelative(p);
  }

  Index reloaded() {
    Index idx(repo_);
    idx.load();
    return idx;
  }

  TempDir tmp_;
  Repository repo_;
  std::unique_ptr<CwdGuard> cwd_;
};

} // namespace

TEST_F(IndexTest, FreshRepositoryHasEmptyIndex) {
  EXPECT_EQ(reloaded().size(), 0u);
}

TEST_F(IndexTest, SaveThenLoadRoundTripsEveryEntry) {
  Index idx(repo_);
  idx.stage(path("a.txt"), entry('a', "100644"));
  idx.stage(path("dir/b.sh"), entry('b', "100755"));
  idx.stage(path("dir/sub/link"), entry('c', "120000"));
  idx.save();

  Index loaded = reloaded();
  EXPECT_EQ(loaded.entries(), idx.entries());
}

TEST_F(IndexTest, StagingSamePathReplacesEntry) {
  Index idx(repo_);
  idx.stage(path("a"), entry('1', "100644"));
  idx.stage(path("a"), entry('2', "100755"));
  ASSERT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.find(path("a"))->hash, fakeHash('2'));
  EXPECT_EQ(idx.find(path("a"))->mode, ark::FileMode::Executable);
}

TEST_F(IndexTest, EraseDropsEntriesAndPersists) {
  Index idx(repo_);
  idx.stage(path("keep"), entry('a', "100644"));
  idx.stage(path("gone1"), entry('1', "100644"));
  idx.stage(path("gone2"), entry('2', "100644"));
  idx.erase(path("gone1"));
  idx.erase(path("gone2"));
  idx.save();

  Index loaded = reloaded();
  EXPECT_TRUE(loaded.contains(path("keep")));
  EXPECT_FALSE(loaded.contains(path("gone1")));
  EXPECT_FALSE(loaded.contains(path("gone2")));
}

TEST_F(IndexTest, FindReturnsNullForUnknownPath) {
  Index idx(repo_);
  EXPECT_EQ(idx.find(path("nope")), nullptr);
}

TEST_F(IndexTest, ReplaceAllReplacesAndStageAllMerges) {
  Index idx(repo_);
  idx.stage(path("old"), entry('e', "100644"));
  idx.replaceAll({{path("new"), entry('d', "100644")}});
  EXPECT_FALSE(idx.contains(path("old")));
  EXPECT_TRUE(idx.contains(path("new")));

  idx.stageAll({{path("extra"), entry('e', "100755")}});
  EXPECT_TRUE(idx.contains(path("new")));
  EXPECT_TRUE(idx.contains(path("extra")));
  EXPECT_EQ(idx.find(path("extra"))->mode, ark::FileMode::Executable);
}

TEST_F(IndexTest, ManyEntriesRoundTrip) {
  Index idx(repo_);
  for (int i = 0; i < 2000; i++) {
    idx.stage(path("d" + std::to_string(i % 17) + "/f" + std::to_string(i)),
              entry("0123456789abcdef"[i % 16], i % 3 ? "100644" : "100755"));
  }
  idx.save();
  EXPECT_EQ(reloaded().entries(), idx.entries());
}

// tmp/bugs/05
TEST_F(IndexTest, PathsWithSpacesRoundTrip) {
  Index idx(repo_);
  idx.stage(path("my file.txt"), entry('a', "100644"));
  idx.stage(path("dir with spaces/x  y.txt"), entry('b', "100644"));
  idx.stage(path("after.txt"), entry('c', "100644"));
  idx.save();
  EXPECT_EQ(reloaded().entries(), idx.entries());
}

TEST_F(IndexTest, IndexFileWritesSortedLinesAndParsesThemBack) {
  ark::FlatTree tree = {{path("b"), entry('b', "100755")},
                        {path("a"), entry('a', "100644")}};
  std::ostringstream out;
  ark::IndexFile::write(out, tree);
  EXPECT_EQ(out.str(), "100644 " + std::string(64, 'a') + " a\n100755 " +
                           std::string(64, 'b') + " b\n");

  std::istringstream in(out.str());
  EXPECT_EQ(ark::IndexFile::parse(in), tree);
}
