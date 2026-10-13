// Randomized end-to-end test.
//
// For each seed: build several branches with random file trees (nested dirs,
// shared basenames, binary and empty files, executable bits), commit them, and
// then hop between branches in random order. After every switch the working
// tree must equal that branch's snapshot byte for byte, status must be clean,
// and the index must match the commit.
//
// Run more seeds:   ARK_SEEDS=200 ./test_ark --gtest_filter='RandomHistory*'
// Replay one seed:  ARK_SEED=12345 ./test_ark --gtest_filter='RandomHistory*'
#include "support/ark_test.h"

#include <cstdlib>
#include <random>

using namespace arktest;

namespace {

class Generator {
public:
  explicit Generator(uint32_t seed) : rng_(seed) {}

  size_t below(size_t n) { return std::uniform_int_distribution<size_t>(0, n - 1)(rng_); }
  bool chance(double p) { return std::bernoulli_distribution(p)(rng_); }

  std::string path() {
    // A small name pool makes collisions like a/x.txt vs b/x.txt likely.
    static const std::vector<std::string> dirs = {"", "", "src", "src/lib", "docs", "a", "a/b", "a/b/c"};
    static const std::vector<std::string> names = {"x.txt", "README", "main.cpp", "data.bin", "y", "z.md", "empty"};
    std::string d = dirs[below(dirs.size())];
    std::string n = names[below(names.size())];
    return d.empty() ? n : d + "/" + n;
  }

  std::string content() {
    switch (below(5)) {
    case 0:
      return "";
    case 1: { // binary with NULs
      std::string s(below(512) + 1, '\0');
      for (auto &c : s) {
        c = static_cast<char>(below(256));
      }
      return s;
    }
    default: { // text
      std::string s;
      size_t lines = below(20) + 1;
      for (size_t i = 0; i < lines; i++) {
        s += "line " + std::to_string(below(1000)) + "\n";
      }
      return s;
    }
    }
  }

private:
  std::mt19937 rng_;
};

// Adds, modifies, deletes and chmods files in the sandbox.
void mutate(Sandbox &sb, Generator &gen, size_t ops) {
  for (size_t i = 0; i < ops; i++) {
    Snapshot current = sb.worktree();
    current.erase(".arkignore");
    const size_t kind = gen.below(4);
    if (current.empty() || kind == 0) {
      std::string p = gen.path();
      // Don't create a file where a directory exists or vice versa;
      // file<->directory changes are covered by a dedicated test.
      bool clash = false;
      for (const auto &[existing, st] : current) {
        if (existing.rfind(p + "/", 0) == 0 || p.rfind(existing + "/", 0) == 0) {
          clash = true;
        }
      }
      if (!clash) {
        sb.write(p, gen.content());
        if (gen.chance(0.2)) {
          setExecutable(sb.repo() / p, true);
        }
      }
      continue;
    }
    auto it = current.begin();
    std::advance(it, gen.below(current.size()));
    const std::string &p = it->first;
    if (kind == 1) {
      sb.write(p, gen.content());
    } else if (kind == 2) {
      fs::remove(sb.repo() / p);
    } else {
      setExecutable(sb.repo() / p, !it->second.executable);
    }
  }
}

std::vector<uint32_t> seeds() {
  if (const char *one = std::getenv("ARK_SEED")) {
    return {static_cast<uint32_t>(std::strtoul(one, nullptr, 10))};
  }
  size_t n = 8;
  if (const char *count = std::getenv("ARK_SEEDS")) {
    n = std::strtoul(count, nullptr, 10);
  }
  std::vector<uint32_t> out;
  for (uint32_t i = 1; i <= n; i++) {
    out.push_back(i * 2654435761u); // spread seeds out
  }
  return out;
}

class RandomHistory : public ::testing::TestWithParam<uint32_t> {};

} // namespace

TEST_P(RandomHistory, SwitchingReproducesEveryBranchExactly) {
  const uint32_t seed = GetParam();
  SCOPED_TRACE("replay with ARK_SEED=" + std::to_string(seed));
  Generator gen(seed);
  Sandbox sb;
  ASSERT_NO_FATAL_FAILURE(sb.initRepo());
  sb.write(".arkignore", ".ark/\n.ark.log\n");

  std::map<std::string, Snapshot> snapshots;
  std::map<std::string, std::string> heads;

  // main: a random initial tree
  mutate(sb, gen, 12);
  ASSERT_EQ(sb.ark({"add", "."}).exitCode, 0);
  ASSERT_EQ(sb.commit("initial").exitCode, 0);
  snapshots["main"] = sb.worktree();
  heads["main"] = sb.headCommit();

  // three more branches, each forked from a random existing branch
  for (int b = 1; b <= 3; b++) {
    auto from = snapshots.begin();
    std::advance(from, gen.below(snapshots.size()));
    const std::string name = "branch" + std::to_string(b);
    ASSERT_EQ(sb.ark({"switch", from->first}).exitCode, 0);
    ASSERT_EQ(sb.worktree(), from->second) << "switch to " << from->first;
    ASSERT_EQ(sb.ark({"branch", name}).exitCode, 0);
    ASSERT_EQ(sb.ark({"switch", name}).exitCode, 0);

    mutate(sb, gen, 6);
    ASSERT_EQ(sb.ark({"add", "."}).exitCode, 0);
    RunResult c = sb.commit("work on " + name);
    ASSERT_EQ(c.exitCode, 0) << c;
    snapshots[name] = sb.worktree();
    heads[name] = sb.headCommit();

    // The commit must store exactly the working tree.
    FileMap stored = sb.commitFiles(heads[name]);
    ASSERT_EQ(stored.size(), snapshots[name].size());
    for (const auto &[path, st] : snapshots[name]) {
      ASSERT_TRUE(stored.count(path)) << path;
      EXPECT_EQ(stored[path].first, blobHash(st.content)) << path;
      EXPECT_EQ(stored[path].second, st.executable ? "100755" : "100644") << path;
    }
  }

  // hop around
  for (int hop = 0; hop < 10; hop++) {
    auto target = snapshots.begin();
    std::advance(target, gen.below(snapshots.size()));
    SCOPED_TRACE("hop " + std::to_string(hop) + " -> " + target->first);

    RunResult r = sb.ark({"switch", target->first});
    ASSERT_LT(r.exitCode, 2) << r; // 0, or 1 for "already on branch"
    Snapshot now = sb.worktree();
    ASSERT_EQ(now, target->second) << "expected:\n"
                                   << describe(target->second) << "actual:\n"
                                   << describe(now);
    EXPECT_EQ(sb.headCommit(), heads[target->first]);
    EXPECT_EQ(sb.index(), sb.commitFiles(heads[target->first]));
    Status st = sb.status();
    EXPECT_TRUE(st.clean()) << st.raw;
  }
}

INSTANTIATE_TEST_SUITE_P(Seeds, RandomHistory, ::testing::ValuesIn(seeds()));
