#include "support/ark_test.h"

#include <ark/core/exceptions.h>
#include <ark/objects/flat_tree.h>
#include <ark/objects/tree.h>
#include <ark/objects/tree_builder.h>
#include <ark/storage/object_store.h>

using namespace arktest;

namespace {

ark::ObjectHash hashOfByte(char c) {
  return ark::ObjectStore::hashOf(ark::ObjectType::Blob, std::string(1, c));
}

ark::FileEntry entryFor(char c, ark::FileMode mode = ark::FileMode::Regular) {
  return ark::FileEntry{hashOfByte(c), mode};
}

class TreeValueTest : public ::testing::Test {
protected:
  ark::ObjectStore store() { return ark::ObjectStore(tmp_.path() / "objects"); }
  TempDir tmp_;
};

} // namespace

TEST_F(TreeValueTest, SerializeThenParseRoundTrips) {
  ark::Tree tree;
  tree.entries.push_back({"a.txt", ark::FileMode::Regular,
                          ark::ObjectType::Blob, hashOfByte('a')});
  tree.entries.push_back({"run.sh", ark::FileMode::Executable,
                          ark::ObjectType::Blob, hashOfByte('b')});
  tree.entries.push_back({"sub", ark::FileMode::Directory,
                          ark::ObjectType::Tree, hashOfByte('c')});

  const std::string body = tree.serialize();
  ark::Tree parsed = ark::Tree::parse(body);

  ASSERT_EQ(parsed.entries.size(), 3u);
  for (size_t i = 0; i < 3; i++) {
    EXPECT_EQ(parsed.entries[i].name, tree.entries[i].name);
    EXPECT_EQ(parsed.entries[i].mode, tree.entries[i].mode);
    EXPECT_EQ(parsed.entries[i].type, tree.entries[i].type);
    EXPECT_EQ(parsed.entries[i].hash, tree.entries[i].hash);
  }
  EXPECT_EQ(parsed.serialize(), body);
}

TEST_F(TreeValueTest, SerializeUsesTheLineFormat) {
  ark::Tree tree;
  tree.entries.push_back({"f", ark::FileMode::Regular, ark::ObjectType::Blob,
                          hashOfByte('f')});
  EXPECT_EQ(tree.serialize(), "100644 blob " + hashOfByte('f').hex() + " f\n");
}

TEST_F(TreeValueTest, EntriesAreSortedByNameRegardlessOfInsertionOrder) {
  ark::Tree forward;
  ark::Tree backward;
  for (char c : std::string("abcd"))
    forward.entries.push_back({std::string(1, c), ark::FileMode::Regular,
                               ark::ObjectType::Blob, hashOfByte(c)});
  for (char c : std::string("dcba"))
    backward.entries.push_back({std::string(1, c), ark::FileMode::Regular,
                                ark::ObjectType::Blob, hashOfByte(c)});

  EXPECT_EQ(forward.serialize(), backward.serialize());
  ark::Tree parsed = ark::Tree::parse(backward.serialize());
  ASSERT_EQ(parsed.entries.size(), 4u);
  EXPECT_EQ(parsed.entries[0].name, "a");
  EXPECT_EQ(parsed.entries[3].name, "d");
}

TEST_F(TreeValueTest, ParseRejectsMalformedEntries) {
  EXPECT_THROW(ark::Tree::parse("100644 blob onlythree\n"),
               InvalidObjectException);
  EXPECT_THROW(ark::Tree::parse("100644 widget " + hashOfByte('a').hex() +
                                " x\n"),
               InvalidObjectException);
}

TEST_F(TreeValueTest, FlattenTreeReturnsOnlyBlobsKeyedByFullPath) {
  auto s = store();
  ark::TreeBuilder builder;
  builder.add(ark::RepoPath::fromRelative("top.txt"), entryFor('t'));
  builder.add(ark::RepoPath::fromRelative("d/a.txt"), entryFor('a'));
  builder.add(ark::RepoPath::fromRelative("d/e/a.txt"),
              entryFor('b', ark::FileMode::Executable));
  const ark::ObjectHash root = builder.write(s);

  ark::FlatTree flat = ark::flattenTree(s, root);

  ark::FlatTree expected;
  expected.insert({ark::RepoPath::fromRelative("top.txt"), entryFor('t')});
  expected.insert({ark::RepoPath::fromRelative("d/a.txt"), entryFor('a')});
  expected.insert({ark::RepoPath::fromRelative("d/e/a.txt"),
                   entryFor('b', ark::FileMode::Executable)});
  EXPECT_EQ(flat, expected);
}

TEST_F(TreeValueTest, BuilderHashDoesNotDependOnAddOrder) {
  auto s = store();
  ark::TreeBuilder one;
  one.add(ark::RepoPath::fromRelative("x/1"), entryFor('1'));
  one.add(ark::RepoPath::fromRelative("a"), entryFor('a'));
  one.add(ark::RepoPath::fromRelative("x/2"), entryFor('2'));
  ark::TreeBuilder two;
  two.add(ark::RepoPath::fromRelative("x/2"), entryFor('2'));
  two.add(ark::RepoPath::fromRelative("a"), entryFor('a'));
  two.add(ark::RepoPath::fromRelative("x/1"), entryFor('1'));
  EXPECT_EQ(one.write(s), two.write(s));
}

TEST_F(TreeValueTest, EmptyBuilderWritesTheEmptyTree) {
  auto s = store();
  ark::TreeBuilder builder;
  const ark::ObjectHash root = builder.write(s);

  EXPECT_EQ(root, ark::ObjectStore::hashOf(ark::ObjectType::Tree, ""));
  auto raw = s.read(root);
  ASSERT_TRUE(raw);
  EXPECT_EQ(raw->body, "");
  EXPECT_TRUE(ark::flattenTree(s, root).empty());
}
