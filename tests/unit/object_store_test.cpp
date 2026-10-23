#include "support/ark_test.h"

#include <ark/core/exceptions.h>
#include <ark/objects/blob.h>
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>
#include <ark/storage/sha256.h>

using namespace arktest;

namespace {

class ObjectStoreTest : public ::testing::Test {
protected:
  ark::ObjectStore store() { return ark::ObjectStore(tmp_.path() / "objects"); }
  TempDir tmp_;
};

} // namespace

TEST_F(ObjectStoreTest, WriteThenReadRoundTrips) {
  auto s = store();
  ark::ObjectHash h = s.write(ark::ObjectType::Blob, "hello world\n");
  auto raw = s.read(h);
  ASSERT_TRUE(raw.has_value());
  EXPECT_EQ(raw->type, ark::ObjectType::Blob);
  EXPECT_EQ(raw->body, "hello world\n");
}

TEST_F(ObjectStoreTest, HashOfMatchesWrite) {
  auto s = store();
  ark::ObjectHash h = s.write(ark::ObjectType::Tree, "abc");
  EXPECT_EQ(ark::ObjectStore::hashOf(ark::ObjectType::Tree, "abc"), h);
}

TEST_F(ObjectStoreTest, ContainsBeforeAndAfterWrite) {
  auto s = store();
  ark::ObjectHash h = ark::ObjectStore::hashOf(ark::ObjectType::Blob, "x");
  EXPECT_FALSE(s.contains(h));
  EXPECT_FALSE(s.read(h).has_value());
  s.write(ark::ObjectType::Blob, "x");
  EXPECT_TRUE(s.contains(h));
}

TEST_F(ObjectStoreTest, WritingTwiceIsIdempotent) {
  auto s = store();
  EXPECT_EQ(s.write(ark::ObjectType::Commit, "c"),
            s.write(ark::ObjectType::Commit, "c"));
}

TEST(ObjectFormatTest, FrameAndParseRoundTrip) {
  std::string framed = ark::frameObject(ark::ObjectType::Blob, "data");
  EXPECT_EQ(framed, std::string("blob 4\0data", 11));
  auto h = ark::ObjectHash::zero();
  ark::RawObject raw = ark::parseObjectFrame(h, framed);
  EXPECT_EQ(raw.type, ark::ObjectType::Blob);
  EXPECT_EQ(raw.body, "data");
}

TEST(ObjectFormatTest, ParseRejectsMalformedFrames) {
  auto h = ark::ObjectHash::zero();
  EXPECT_THROW(ark::parseObjectFrame(h, "blob 4"), ObjectFormatException);
  EXPECT_THROW(ark::parseObjectFrame(h, std::string("blob\0x", 6)),
               ObjectFormatException);
  EXPECT_THROW(ark::parseObjectFrame(h, std::string("tag 1\0x", 7)),
               ObjectFormatException);
  EXPECT_THROW(ark::parseObjectFrame(h, std::string("blob 9\0x", 8)),
               ObjectFormatException);
}

TEST(Sha256Test, KnownVector) {
  EXPECT_EQ(ark::sha256Hex("abc"),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

// tmp/bugs/10
TEST(BlobReadTest, TryReadReturnsNothingForMissingFile) {
  EXPECT_FALSE(ark::Blob::tryReadFromFile("/nonexistent/ark/blob.txt").has_value());
}
