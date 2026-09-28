#include "support/ark_test.h"

#include <ark/repo/index.h>
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

  static std::string fakeHash(char c) { return std::string(64, c); }

  Index reloaded() {
    Index idx;
    idx.load();
    return idx;
  }

  TempDir tmp_;
  std::unique_ptr<CwdGuard> cwd_;
};

} // namespace

TEST_F(IndexTest, FreshRepositoryHasEmptyIndex) {
  EXPECT_EQ(reloaded().size(), 0u);
}

TEST_F(IndexTest, SaveThenLoadRoundTripsEveryEntry) {
  Index idx;
  idx.stage("a.txt", fakeHash('a'), "100644");
  idx.stage("dir/b.sh", fakeHash('b'), "100755");
  idx.stage("dir/sub/link", fakeHash('c'), "120000");
  idx.save();

  Index loaded = reloaded();
  EXPECT_EQ(loaded.toMap(), idx.toMap());
}

TEST_F(IndexTest, StagingSamePathReplacesEntry) {
  Index idx;
  idx.stage("a", fakeHash('1'), "100644");
  idx.stage("a", fakeHash('2'), "100755");
  ASSERT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.get("a")->hash, fakeHash('2'));
  EXPECT_EQ(idx.get("a")->mode, "100755");
}

TEST_F(IndexTest, RemoveAndUnstageDropEntriesAndPersist) {
  Index idx;
  idx.stage("keep", fakeHash('k'), "100644");
  idx.stage("gone1", fakeHash('1'), "100644");
  idx.stage("gone2", fakeHash('2'), "100644");
  idx.remove("gone1");
  idx.unstage("gone2");
  idx.save();

  Index loaded = reloaded();
  EXPECT_TRUE(loaded.contains("keep"));
  EXPECT_FALSE(loaded.contains("gone1"));
  EXPECT_FALSE(loaded.contains("gone2"));
}

TEST_F(IndexTest, GetReturnsNullForUnknownPath) {
  Index idx;
  EXPECT_EQ(idx.get("nope"), nullptr);
}

TEST_F(IndexTest, SetAllReplacesAndFromMapMerges) {
  Index idx;
  idx.stage("old", fakeHash('o'), "100644");
  idx.setAll({{"new", IndexEntry{fakeHash('n'), "100644"}}});
  EXPECT_FALSE(idx.contains("old"));
  EXPECT_TRUE(idx.contains("new"));

  idx.fromMap({{"extra", {fakeHash('e'), "100755"}}});
  EXPECT_TRUE(idx.contains("new"));
  EXPECT_TRUE(idx.contains("extra"));
  EXPECT_EQ(idx.get("extra")->mode, "100755");
}

TEST_F(IndexTest, ManyEntriesRoundTrip) {
  Index idx;
  for (int i = 0; i < 2000; i++) {
    idx.stage("d" + std::to_string(i % 17) + "/f" + std::to_string(i),
              fakeHash("0123456789abcdef"[i % 16]), i % 3 ? "100644" : "100755");
  }
  idx.save();
  EXPECT_EQ(reloaded().toMap(), idx.toMap());
}

// tmp/bugs/05
TEST_F(IndexTest, PathsWithSpacesRoundTrip) {
  Index idx;
  idx.stage("my file.txt", fakeHash('a'), "100644");
  idx.stage("dir with spaces/x  y.txt", fakeHash('b'), "100644");
  idx.stage("after.txt", fakeHash('c'), "100644");
  idx.save();
  EXPECT_EQ(reloaded().toMap(), idx.toMap());
}
