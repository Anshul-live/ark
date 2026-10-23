#include "support/ark_test.h"

#include <ark/core/file_mode.h>
#include <ark/core/object_hash.h>
#include <ark/core/object_type.h>
#include <ark/core/ref_name.h>
#include <ark/core/repo_path.h>

#include <sys/stat.h>

#include <fstream>
#include <sstream>
#include <unordered_set>

using namespace arktest;

TEST(CoreObjectHash, ParsesValidHex) {
  std::string hex(64, 'a');
  auto h = ark::ObjectHash::parse(hex);
  ASSERT_TRUE(h.has_value());
  EXPECT_EQ(h->hex(), hex);
  EXPECT_FALSE(h->isZero());
}

TEST(CoreObjectHash, RejectsInvalid) {
  EXPECT_FALSE(ark::ObjectHash::parse(std::string(64, 'A')));
  EXPECT_FALSE(ark::ObjectHash::parse(std::string(63, 'a')));
  EXPECT_FALSE(ark::ObjectHash::parse(std::string(65, 'a')));
  EXPECT_FALSE(ark::ObjectHash::parse(std::string(63, 'a') + "g"));
  EXPECT_FALSE(ark::ObjectHash::parse(""));
}

TEST(CoreObjectHash, ZeroIsAllZeroes) {
  auto z = ark::ObjectHash::zero();
  EXPECT_EQ(z.hex(), std::string(64, '0'));
  EXPECT_TRUE(z.isZero());
  EXPECT_EQ(z, *ark::ObjectHash::parse(std::string(64, '0')));
}

TEST(CoreObjectHash, FanoutSplit) {
  std::string hex = "ab" + std::string(62, 'c');
  auto h = *ark::ObjectHash::parse(hex);
  EXPECT_EQ(h.fanoutDir(), "ab");
  EXPECT_EQ(h.fanoutFile(), std::string(62, 'c'));
}

TEST(CoreObjectHash, FromDigestRoundTrip) {
  std::array<unsigned char, 32> d{};
  for (size_t i = 0; i < d.size(); ++i)
    d[i] = static_cast<unsigned char>(i * 8 + 1);
  auto h = ark::ObjectHash::fromDigest(d);
  EXPECT_EQ(h.hex().size(), 64u);
  EXPECT_EQ(h.hex().substr(0, 6), "010911");
  EXPECT_EQ(ark::ObjectHash::parse(h.hex()), h);
}

TEST(CoreObjectHash, OrderingHashingAndStreaming) {
  auto a = *ark::ObjectHash::parse(std::string(64, '1'));
  auto b = *ark::ObjectHash::parse(std::string(64, '2'));
  EXPECT_NE(a, b);
  EXPECT_LT(a, b);
  std::unordered_set<ark::ObjectHash> set{a, b, a};
  EXPECT_EQ(set.size(), 2u);
  std::ostringstream os;
  os << a;
  EXPECT_EQ(os.str(), a.hex());
}

TEST(CoreObjectType, ToStringAndParse) {
  EXPECT_EQ(ark::toString(ark::ObjectType::Blob), "blob");
  EXPECT_EQ(ark::toString(ark::ObjectType::Tree), "tree");
  EXPECT_EQ(ark::toString(ark::ObjectType::Commit), "commit");
  EXPECT_EQ(ark::parseObjectType("blob"), ark::ObjectType::Blob);
  EXPECT_EQ(ark::parseObjectType("tree"), ark::ObjectType::Tree);
  EXPECT_EQ(ark::parseObjectType("commit"), ark::ObjectType::Commit);
  EXPECT_FALSE(ark::parseObjectType("tag"));
  EXPECT_FALSE(ark::parseObjectType(""));
}

TEST(CoreFileMode, OctalAndParse) {
  using ark::FileMode;
  for (auto m : {FileMode::Regular, FileMode::Executable, FileMode::Symlink,
                 FileMode::Directory})
    EXPECT_EQ(ark::parseFileMode(ark::toOctalString(m)), m);
  EXPECT_EQ(ark::toOctalString(FileMode::Regular), "100644");
  EXPECT_EQ(ark::toOctalString(FileMode::Executable), "100755");
  EXPECT_EQ(ark::toOctalString(FileMode::Symlink), "120000");
  EXPECT_EQ(ark::toOctalString(FileMode::Directory), "040000");
  EXPECT_FALSE(ark::parseFileMode("100664"));
  EXPECT_FALSE(ark::parseFileMode(""));
}

TEST(CoreFileMode, DetectsRealFiles) {
  TempDir tmp;
  auto plain = tmp.path() / "plain";
  auto exe = tmp.path() / "exe";
  std::ofstream(plain) << "x";
  std::ofstream(exe) << "x";
  ASSERT_EQ(chmod(plain.c_str(), 0644), 0);
  ASSERT_EQ(chmod(exe.c_str(), 0755), 0);
  EXPECT_EQ(ark::detectFileMode(plain), ark::FileMode::Regular);
  EXPECT_EQ(ark::detectFileMode(exe), ark::FileMode::Executable);
  EXPECT_EQ(ark::detectFileMode(tmp.path()), ark::FileMode::Directory);
  auto link = tmp.path() / "link";
  std::filesystem::create_symlink(plain, link);
  EXPECT_EQ(ark::detectFileMode(link), ark::FileMode::Symlink);
  EXPECT_FALSE(ark::detectFileMode(tmp.path() / "missing"));
}

TEST(CoreRepoPath, Normalises) {
  EXPECT_EQ(ark::RepoPath::fromRelative("a//b/./c").str(), "a/b/c");
  EXPECT_EQ(ark::RepoPath::fromRelative("a/../b").str(), "b");
  EXPECT_EQ(ark::RepoPath::fromRelative("./x/").str(), "x");
  EXPECT_EQ(ark::RepoPath::fromRelative("").str(), "");
  EXPECT_EQ(ark::RepoPath::fromRelative("a/..").str(), "");
}

TEST(CoreRepoPath, FromAbsolute) {
  std::filesystem::path root = "/tmp/repo";
  auto p = ark::RepoPath::fromAbsolute("/tmp/repo/src/a.txt", root);
  ASSERT_TRUE(p.has_value());
  EXPECT_EQ(p->str(), "src/a.txt");
  EXPECT_FALSE(ark::RepoPath::fromAbsolute("/tmp/other/a.txt", root));
  EXPECT_FALSE(ark::RepoPath::fromAbsolute("/tmp/repo/../x", root));
  EXPECT_EQ(p->toAbsolute(root), std::filesystem::path("/tmp/repo/src/a.txt"));
}

TEST(CoreRepoPath, ChildComponentsAndArkDir) {
  ark::RepoPath empty;
  EXPECT_EQ(empty.child("a").str(), "a");
  auto p = ark::RepoPath::fromRelative("a/b").child("c");
  EXPECT_EQ(p.str(), "a/b/c");
  EXPECT_EQ(p.components(), (std::vector<std::string>{"a", "b", "c"}));
  EXPECT_FALSE(p.isInsideArkDir());
  EXPECT_TRUE(ark::RepoPath::fromRelative(".ark/HEAD").isInsideArkDir());
  EXPECT_TRUE(ark::RepoPath::fromRelative(".ark").isInsideArkDir());
  EXPECT_FALSE(ark::RepoPath::fromRelative(".arkx/y").isInsideArkDir());
  EXPECT_LT(ark::RepoPath::fromRelative("a"), ark::RepoPath::fromRelative("b"));
  std::unordered_set<ark::RepoPath> set{p, p};
  EXPECT_EQ(set.size(), 1u);
}

TEST(CoreRefName, BranchParse) {
  EXPECT_TRUE(ark::BranchName::parse("main"));
  EXPECT_TRUE(ark::BranchName::parse("feature/x-1"));
  for (const char *bad : {"", "a..b", "a b", "a~b", "a^b", "a:b", "a?b", "a*b",
                          "a[b", "a\\b", "-x", "/x", "x/", "x.lock", "a\tb"})
    EXPECT_FALSE(ark::BranchName::parse(bad)) << bad;
}

TEST(CoreRefName, RefParseAndBranch) {
  EXPECT_FALSE(ark::RefName::parse("heads/main"));
  EXPECT_FALSE(ark::RefName::parse("refs/heads/a..b"));
  auto r = ark::RefName::parse("refs/heads/feature/x");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->asBranch().has_value());
  EXPECT_EQ(r->asBranch()->str(), "feature/x");
  auto tag = ark::RefName::parse("refs/tags/v1");
  ASSERT_TRUE(tag.has_value());
  EXPECT_FALSE(tag->asBranch());
  EXPECT_EQ(ark::BranchName::unchecked("main").toRefName().str(),
            "refs/heads/main");
  EXPECT_LT(ark::BranchName::unchecked("a"), ark::BranchName::unchecked("b"));
}

TEST(CoreRefName, HeadTargetHoldsEither) {
  ark::HeadTarget sym = ark::RefName::unchecked("refs/heads/main");
  ark::HeadTarget det = ark::ObjectHash::zero();
  EXPECT_TRUE(std::holds_alternative<ark::RefName>(sym));
  EXPECT_TRUE(std::holds_alternative<ark::ObjectHash>(det));
}
