#include "support/ark_test.h"

#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/storage/object_store.h>

using namespace arktest;

namespace {

const std::string kTree(64, 'a');
const std::string kParentA(64, 'b');
const std::string kParentB(64, 'c');

ark::Commit sample(std::vector<ark::ObjectHash> parents) {
  ark::Commit c;
  c.tree = ark::ObjectHash::unchecked(kTree);
  c.parents = std::move(parents);
  c.committer = ark::Signature{{"Some Body", "some@body.test"}, 1700000000, "+0530"};
  c.message = "line one\n\nline three";
  return c;
}

} // namespace

TEST(CommitValueTest, SignatureSerializeFormat) {
  ark::Signature sig{{"Ada Lovelace", "ada@example.com"}, 1234567890, "-0700"};
  EXPECT_EQ(sig.serialize(), "Ada Lovelace <ada@example.com> 1234567890 -0700");
}

TEST(CommitValueTest, SerializeLayout) {
  ark::Commit c = sample({ark::ObjectHash::unchecked(kParentA)});
  EXPECT_EQ(c.serialize(),
            "tree " + kTree + "\nparent " + kParentA +
                "\ncommitter Some Body <some@body.test> 1700000000 +0530\n"
                "line one\n\nline three\n");
}

TEST(CommitValueTest, RoundTripSingleParent) {
  ark::Commit c = sample({ark::ObjectHash::unchecked(kParentA)});
  EXPECT_EQ(ark::Commit::parse(c.serialize()), c);
}

TEST(CommitValueTest, RootCommitWithoutParentsHasNoParentLine) {
  ark::Commit c = sample({});
  EXPECT_EQ(c.serialize().find("parent"), std::string::npos);
  EXPECT_EQ(ark::Commit::parse(c.serialize()), c);
}

TEST(CommitValueTest, RootCommitKeepsTheNullParentLine) {
  ark::Commit c = sample({ark::ObjectHash::zero()});
  EXPECT_NE(c.serialize().find("parent " + std::string(64, '0') + "\n"),
            std::string::npos);
  EXPECT_EQ(ark::Commit::parse(c.serialize()), c);
}

TEST(CommitValueTest, RoundTripTwoParents) {
  ark::Commit c = sample({ark::ObjectHash::unchecked(kParentA),
                          ark::ObjectHash::unchecked(kParentB)});
  ark::Commit parsed = ark::Commit::parse(c.serialize());
  ASSERT_EQ(parsed.parents.size(), 2u);
  EXPECT_EQ(parsed.parents[0].hex(), kParentA);
  EXPECT_EQ(parsed.parents[1].hex(), kParentB);
  EXPECT_EQ(parsed, c);
}

TEST(CommitValueTest, MessageLookingLikeAHeaderStaysInTheMessage) {
  ark::Commit c = sample({});
  c.message = "parent " + kParentA;
  EXPECT_EQ(ark::Commit::parse(c.serialize()), c);
}

TEST(CommitValueTest, StoreWriteThenReadReturnsEqualCommit) {
  TempDir tmp;
  ark::ObjectStore store(tmp.path() / "objects");
  ark::CommitStore commits(store);
  ark::Commit c = sample({ark::ObjectHash::unchecked(kParentA),
                          ark::ObjectHash::unchecked(kParentB)});
  ark::ObjectHash h = commits.write(c);
  EXPECT_EQ(h, ark::ObjectStore::hashOf(ark::ObjectType::Commit, c.serialize()));
  EXPECT_EQ(commits.read(h), c);
}

TEST(CommitValueTest, StoreReadOfMissingObjectThrows) {
  TempDir tmp;
  ark::ObjectStore store(tmp.path() / "objects");
  ark::CommitStore commits(store);
  EXPECT_ANY_THROW(commits.read(ark::ObjectHash::unchecked(kTree)));
}
