#include "support/ark_test.h"

#include <ark/objects/tree_diff.h>
#include <ark/storage/object_store.h>

namespace {

ark::FileEntry entryFor(char c, ark::FileMode mode = ark::FileMode::Regular) {
  return ark::FileEntry{
      ark::ObjectStore::hashOf(ark::ObjectType::Blob, std::string(1, c)), mode};
}

ark::RepoPath p(const char *path) { return ark::RepoPath::fromRelative(path); }

} // namespace

TEST(TreeDiffTest, ReportsAddedFiles) {
  ark::FlatTree from;
  ark::FlatTree to{{p("new.txt"), entryFor('a')}};
  auto changes = ark::diffTrees(from, to);
  ASSERT_EQ(changes.size(), 1u);
  EXPECT_EQ(changes[0].kind, ark::ChangeKind::Added);
  EXPECT_EQ(changes[0].path, p("new.txt"));
  EXPECT_FALSE(changes[0].before.has_value());
  EXPECT_EQ(changes[0].after, entryFor('a'));
}

TEST(TreeDiffTest, ReportsDeletedFiles) {
  ark::FlatTree from{{p("gone.txt"), entryFor('a')}};
  auto changes = ark::diffTrees(from, {});
  ASSERT_EQ(changes.size(), 1u);
  EXPECT_EQ(changes[0].kind, ark::ChangeKind::Deleted);
  EXPECT_EQ(changes[0].before, entryFor('a'));
  EXPECT_FALSE(changes[0].after.has_value());
}

TEST(TreeDiffTest, ReportsModifiedByHashOnly) {
  ark::FlatTree from{{p("f"), entryFor('a')}, {p("mode"), entryFor('b')}};
  ark::FlatTree to{{p("f"), entryFor('c')},
                   {p("mode"), entryFor('b', ark::FileMode::Executable)}};
  auto changes = ark::diffTrees(from, to);
  ASSERT_EQ(changes.size(), 1u);
  EXPECT_EQ(changes[0].kind, ark::ChangeKind::Modified);
  EXPECT_EQ(changes[0].path, p("f"));
  EXPECT_EQ(changes[0].before, entryFor('a'));
  EXPECT_EQ(changes[0].after, entryFor('c'));
}

TEST(TreeDiffTest, OmitsUnchangedFiles) {
  ark::FlatTree tree{{p("a"), entryFor('a')}, {p("d/b"), entryFor('b')}};
  EXPECT_TRUE(ark::diffTrees(tree, tree).empty());
}

TEST(TreeDiffTest, OutputIsSortedByPath) {
  ark::FlatTree from{{p("m"), entryFor('a')}, {p("z"), entryFor('b')}};
  ark::FlatTree to{{p("a"), entryFor('c')}, {p("m"), entryFor('d')},
                   {p("q/r"), entryFor('e')}};
  auto changes = ark::diffTrees(from, to);
  ASSERT_EQ(changes.size(), 4u);
  EXPECT_EQ(changes[0].path, p("a"));
  EXPECT_EQ(changes[1].path, p("m"));
  EXPECT_EQ(changes[2].path, p("q/r"));
  EXPECT_EQ(changes[3].path, p("z"));
  EXPECT_EQ(changes[3].kind, ark::ChangeKind::Deleted);
}
