#include "support/ark_test.h"

#include <ark/repo/index.h>
#include <ark/cli/commands.h>
#include <ark/objects/blob.h>
#include <ark/objects/objects.h>
#include <ark/objects/tree_builder.h>
#include <ark/repo/repository.h>
#include <ark/storage/object_store.h>

using namespace arktest;

namespace {
std::string hashFile(const std::string &path) {
  return ark::ObjectStore::hashOf(ark::ObjectType::Blob,
                                  ark::Blob::readFromFile(path).data)
      .hex();
}

std::string storeFile(const std::string &path) {
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  return store
      .write(ark::ObjectType::Blob, ark::Blob::readFromFile(path).data)
      .hex();
}

std::string writeTree() {
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  Index idx;
  idx.load();
  return ark::writeTreeFromIndex(idx, store).hex();
}

void stageBlob(Index &idx, const std::string &path, const std::string &hash) {
  idx.stage(ark::RepoPath::fromRelative(path),
            ark::FileEntry{ark::ObjectHash::parse(hash).value(),
                           ark::FileMode::Regular});
}
} // namespace

namespace {

// Runs each test inside a freshly initialised repository.
class ObjectsTest : public ::testing::Test {
protected:
  void SetUp() override {
    cwd_ = std::make_unique<CwdGuard>(tmp_.path());
    ASSERT_EQ(cmd_init({}), 0);
    writeFile(".ark/config", "[user]\nname = Unit\nemail = unit@test\n");
  }
  void TearDown() override { cwd_.reset(); }

  fs::path repo() const { return tmp_.path(); }

  TempDir tmp_;
  std::unique_ptr<CwdGuard> cwd_;
};

} // namespace

TEST_F(ObjectsTest, BlobHashIsSha256OfHeaderAndContent) {
  writeFile("f.txt", "hello\n");
  EXPECT_EQ(hashFile("f.txt"), blobHash("hello\n"));
}

TEST_F(ObjectsTest, BlobHashCoversBinaryContent) {
  const std::string bytes("\0\x01\xff\n\0tail", 9);
  writeFile("bin", bytes);
  EXPECT_EQ(hashFile("bin"), blobHash(bytes));
}

TEST_F(ObjectsTest, EmptyFileHasWellDefinedHash) {
  writeFile("empty", "");
  EXPECT_EQ(hashFile("empty"), blobHash(""));
}

TEST_F(ObjectsTest, WrittenBlobIsStoredUnderFanoutPathAndVerifies) {
  const std::string content = "stored content\n";
  writeFile("f.txt", content);
  const std::string hash = storeFile("f.txt");

  EXPECT_TRUE(fs::is_regular_file(repo() / ".ark/objects" / hash.substr(0, 2) /
                                  hash.substr(2)));
  auto obj = readObject(repo(), hash); // also checks sha256(object) == id
  ASSERT_TRUE(obj);
  EXPECT_EQ(obj->type, "blob");
  EXPECT_EQ(obj->body, content);
  Repository r;
  EXPECT_TRUE(ark::ObjectStore(r.objectsDir())
                  .contains(ark::ObjectHash::unchecked(hash)));
}

TEST_F(ObjectsTest, TreeWriteMatchesIndexContents) {
  const std::map<std::string, std::string> files = {
      {"a.txt", "A"}, {"d/b.txt", "B"}, {"d/e/c.txt", "C"}, {"z.txt", "Z"}};
  Index idx;
  for (const auto &[path, content] : files) {
    writeFile(path, content);
    stageBlob(idx, path, storeFile(path));
  }
  idx.save();

  FileMap stored = flattenTree(repo(), writeTree());

  FileMap expected;
  for (const auto &[path, content] : files) {
    expected[path] = {blobHash(content), "100644"};
  }
  EXPECT_EQ(stored, expected);
}

// tmp/bugs/01 (first entry dropped) + tmp/bugs/02 (basename keys, dirs included)
TEST_F(ObjectsTest, LoadedTreeFlattensToFullPathsOfEveryBlob) {
  const std::map<std::string, std::string> files = {
      {"a.txt", "A"}, {"b.txt", "B"}, {"d/a.txt", "dA"}, {"d/e/c.txt", "C"}};
  Index idx;
  for (const auto &[path, content] : files) {
    writeFile(path, content);
    stageBlob(idx, path, storeFile(path));
  }
  idx.save();
  const std::string treeHash = writeTree();

  Repository r;
  ark::ObjectStore store(r.objectsDir());
  auto flat = ark::flattenTree(store, ark::ObjectHash::unchecked(treeHash));

  FileMap actual;
  for (const auto &[path, entry] : flat)
    actual[path.str()] = {entry.hash.hex(),
                          std::string(ark::toOctalString(entry.mode))};
  FileMap expected;
  for (const auto &[path, content] : files) {
    expected[path] = {blobHash(content), "100644"};
  }
  EXPECT_EQ(actual, expected);
}

// tmp/bugs/04
TEST_F(ObjectsTest, TreeHashDoesNotDependOnStagingOrder) {
  std::vector<std::string> names;
  for (char c = 'a'; c <= 'p'; c++) {
    names.push_back(std::string(1, c) + ".txt");
    writeFile(names.back(), std::string(1, c));
  }
  auto treeFor = [&](const std::vector<std::string> &order) {
    Index idx;
    for (const auto &n : order) {
      stageBlob(idx, n, storeFile(n));
    }
    idx.save();
    return writeTree();
  };
  std::vector<std::string> reversed(names.rbegin(), names.rend());
  EXPECT_EQ(treeFor(names), treeFor(reversed));
}

// tmp/bugs/25
TEST_F(ObjectsTest, CommitCreateRecordsTreeParentsAndMessage) {
  writeFile("f", "x");
  Index idx;
  stageBlob(idx, "f", storeFile("f"));
  idx.save();
  const std::string tree = writeTree();

  Commit *root = Commit::create(tree, "", "", "first");
  CommitInfo info = parseCommit(readObject(repo(), root->hash)->body);
  EXPECT_EQ(info.tree, tree);
  EXPECT_TRUE(info.parents.empty());
  EXPECT_EQ(info.message, "first");
  EXPECT_NE(info.committer.find("Unit <unit@test>"), std::string::npos);

  Commit *child = Commit::create(tree, root->hash, "", "second");
  EXPECT_EQ(parseCommit(readObject(repo(), child->hash)->body).parents,
            std::vector<std::string>{root->hash});
}
