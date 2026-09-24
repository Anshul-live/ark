// Shared helpers for the Ark test suite.
//
// Two rules keep these tests honest:
//   1. Expected values come from an independent implementation (the "oracle"
//      functions below use OpenSSL/zlib directly and parse objects themselves),
//      never from Ark's own code under test.
//   2. Every test runs in its own sandbox: a fresh repo, a private $HOME and
//      $TMPDIR, and no inherited editor variables, so tests can't see each
//      other or the developer's real config.
#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace arktest {

// ---------- filesystem ----------

// A unique directory under the system temp dir, removed on destruction.
// Set ARK_KEEP_TMP=1 to keep it for debugging.
class TempDir {
public:
  TempDir();
  ~TempDir();
  TempDir(const TempDir &) = delete;
  TempDir &operator=(const TempDir &) = delete;
  const fs::path &path() const { return path_; }

private:
  fs::path path_;
};

// Changes the working directory for the lifetime of the guard.
class CwdGuard {
public:
  explicit CwdGuard(const fs::path &dir);
  ~CwdGuard();

private:
  fs::path previous_;
};

void writeFile(const fs::path &path, const std::string &content);
std::string readFile(const fs::path &path);
void setExecutable(const fs::path &path, bool executable);
bool isExecutable(const fs::path &path);
std::string trim(const std::string &s);
std::string stripAnsi(const std::string &s);

// ---------- oracle: independent object-format implementation ----------

std::string sha256Hex(const std::string &data);
// Inflates a zlib stream; throws std::runtime_error on corrupt input.
std::string inflate(const std::string &compressed);
// sha256("blob <size>\0<content>"), the id Ark must give a file.
std::string blobHash(const std::string &content);
bool isHash(const std::string &s);
// Every 64-hex-char token in `text`, in order.
std::vector<std::string> hashesIn(const std::string &text);

struct StoredObject {
  std::string type;
  std::string body;
};
// Reads .ark/objects/<aa>/<rest>, inflates it and splits off the header.
// Returns nullopt if the object file doesn't exist.
std::optional<StoredObject> readObject(const fs::path &repo,
                                       const std::string &hash);

struct TreeEntry {
  std::string mode, type, hash, name;
};
// Parses "<mode> <type> <hash> <name>\n" lines; the name is the rest of the
// line so names containing spaces are supported.
std::vector<TreeEntry> parseTree(const std::string &body);

struct CommitInfo {
  std::string tree;
  std::vector<std::string> parents;
  std::string committer;
  std::string message;
};
CommitInfo parseCommit(const std::string &body);

// Repo-relative path -> (blob hash, mode) for every blob reachable from a tree.
using FileMap = std::map<std::string, std::pair<std::string, std::string>>;
FileMap flattenTree(const fs::path &repo, const std::string &treeHash);

// Parses .ark/index as "<mode> <hash> <path...>" per line.
// If the index format changes, this is the only function to update.
FileMap readIndex(const fs::path &repo);

// ---------- working tree snapshots ----------

struct FileState {
  std::string content; // file bytes, or the link target for symlinks
  bool executable = false;
  bool symlink = false;
  bool operator==(const FileState &o) const {
    return content == o.content && executable == o.executable &&
           symlink == o.symlink;
  }
};
void PrintTo(const FileState &st, std::ostream *os); // readable gtest diffs
using Snapshot = std::map<std::string, FileState>;
// Every file under `root`, excluding .ark/ and stray .ark.log files.
Snapshot snapshot(const fs::path &root);
std::string describe(const Snapshot &s);

// ---------- status output ----------

struct Status {
  std::set<std::string> staged, modified, untracked, deleted;
  std::string raw;
  bool clean() const {
    return staged.empty() && modified.empty() && untracked.empty() &&
           deleted.empty();
  }
};
Status parseStatus(const std::string &output);

// ---------- running the ark binary ----------

struct RunResult {
  int exitCode = -1;
  std::string out;
  std::string err;
  std::string all() const;
};
std::ostream &operator<<(std::ostream &os, const RunResult &r);

// A private sandbox: <tmp>/repo is the working tree, <tmp>/home is $HOME.
class Sandbox {
public:
  Sandbox();

  const fs::path &root() const { return tmp_.path(); }
  fs::path repo() const { return tmp_.path() / "repo"; }
  fs::path home() const { return tmp_.path() / "home"; }

  // Runs ark with `args` in `cwd` (default: the repo root).
  RunResult ark(const std::vector<std::string> &args,
                const fs::path &cwd = {},
                const std::map<std::string, std::string> &extraEnv = {});

  // ark init + a repo-level identity.
  void initRepo(bool withIdentity = true);
  void setRepoIdentity(const std::string &name, const std::string &email);
  // Commits with `message` (fed through a fake $GIT_EDITOR).
  RunResult commit(const std::string &message);
  // add + commit; asserts both succeed and returns the new HEAD commit.
  std::string addAndCommit(const std::vector<std::string> &paths,
                           const std::string &message);

  void write(const std::string &rel, const std::string &content);
  std::string read(const std::string &rel) const;
  bool exists(const std::string &rel) const;
  void remove(const std::string &rel);

  // Reads HEAD and refs directly (not via ark).
  std::string headFile() const;
  std::string headCommit() const;
  std::string branchCommit(const std::string &branch) const;

  std::optional<StoredObject> object(const std::string &hash) const;
  CommitInfo commitInfo(const std::string &hash) const;
  FileMap commitFiles(const std::string &commitHash) const;
  FileMap index() const { return readIndex(repo()); }
  Snapshot worktree() const { return snapshot(repo()); }
  Status status(const fs::path &cwd = {});

private:
  TempDir tmp_;
  fs::path editor_;
};

} // namespace arktest
