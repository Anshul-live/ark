#include "support/ark_test.h"

#include <openssl/evp.h>
#include <zlib.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#ifndef ARK_BIN
#error "ARK_BIN must be defined by the build (path to the ark executable)"
#endif

extern char **environ;

namespace arktest {

// ---------- filesystem ----------

TempDir::TempDir() {
  std::string tmpl =
      (fs::temp_directory_path() / "ark-test-XXXXXX").string();
  if (!mkdtemp(tmpl.data())) {
    throw std::runtime_error(std::string("mkdtemp failed: ") +
                             std::strerror(errno));
  }
  // Resolve symlinks (/var -> /private/var on macOS) so paths compare equal
  // to what the binary sees via current_path().
  path_ = fs::canonical(tmpl);
}

TempDir::~TempDir() {
  if (std::getenv("ARK_KEEP_TMP")) {
    std::cerr << "[ark-test] kept " << path_ << "\n";
    return;
  }
  std::error_code ec;
  // Restore permissions first so chmod 000 test files can be removed.
  for (auto it = fs::recursive_directory_iterator(
           path_, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (!it->is_symlink()) {
      fs::permissions(it->path(), fs::perms::owner_all,
                      fs::perm_options::add, ec);
    }
  }
  fs::remove_all(path_, ec);
}

CwdGuard::CwdGuard(const fs::path &dir) : previous_(fs::current_path()) {
  fs::current_path(dir);
}

CwdGuard::~CwdGuard() {
  std::error_code ec;
  fs::current_path(previous_, ec);
}

void writeFile(const fs::path &path, const std::string &content) {
  if (path.has_parent_path()) {
    fs::create_directories(path.parent_path());
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("cannot write " + path.string());
  }
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::string readFile(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("cannot read " + path.string());
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

void setExecutable(const fs::path &path, bool executable) {
  const auto bits = fs::perms::owner_exec | fs::perms::group_exec |
                    fs::perms::others_exec;
  fs::permissions(path, bits,
                  executable ? fs::perm_options::add
                             : fs::perm_options::remove);
}

bool isExecutable(const fs::path &path) {
  // A missing file reports perms::unknown, which has every bit set; never
  // let that read as "executable".
  const fs::file_status st = fs::symlink_status(path);
  if (!fs::is_regular_file(st)) {
    throw std::runtime_error("isExecutable: not a regular file: " + path.string());
  }
  return (st.permissions() & fs::perms::owner_exec) != fs::perms::none;
}

std::string trim(const std::string &s) {
  const char *ws = " \t\r\n";
  size_t b = s.find_first_not_of(ws);
  if (b == std::string::npos) {
    return "";
  }
  size_t e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

std::string stripAnsi(const std::string &s) {
  static const std::regex ansi("\x1b\\[[0-9;]*m");
  return std::regex_replace(s, ansi, "");
}

// ---------- oracle ----------

std::string sha256Hex(const std::string &data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int len = 0;
  if (EVP_Digest(data.data(), data.size(), digest, &len, EVP_sha256(),
                 nullptr) != 1) {
    throw std::runtime_error("EVP_Digest failed");
  }
  std::ostringstream hex;
  for (unsigned int i = 0; i < len; i++) {
    hex << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(digest[i]);
  }
  return hex.str();
}

std::string inflate(const std::string &compressed) {
  z_stream zs{};
  if (inflateInit(&zs) != Z_OK) {
    throw std::runtime_error("inflateInit failed");
  }
  zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.data()));
  zs.avail_in = static_cast<uInt>(compressed.size());
  std::string out;
  char buf[16384];
  int rc;
  do {
    zs.next_out = reinterpret_cast<Bytef *>(buf);
    zs.avail_out = sizeof(buf);
    rc = ::inflate(&zs, Z_NO_FLUSH);
    if (rc != Z_OK && rc != Z_STREAM_END) {
      inflateEnd(&zs);
      throw std::runtime_error("corrupt zlib stream (rc=" +
                               std::to_string(rc) + ")");
    }
    out.append(buf, sizeof(buf) - zs.avail_out);
  } while (rc != Z_STREAM_END);
  inflateEnd(&zs);
  return out;
}

std::string blobHash(const std::string &content) {
  return sha256Hex("blob " + std::to_string(content.size()) +
                   std::string(1, '\0') + content);
}

bool isHash(const std::string &s) {
  static const std::regex re("^[0-9a-f]{64}$");
  return std::regex_match(s, re);
}

std::vector<std::string> hashesIn(const std::string &text) {
  static const std::regex re("[0-9a-f]{64}");
  std::vector<std::string> out;
  for (auto it = std::sregex_iterator(text.begin(), text.end(), re);
       it != std::sregex_iterator(); ++it) {
    out.push_back(it->str());
  }
  return out;
}

std::optional<StoredObject> readObject(const fs::path &repo,
                                       const std::string &hash) {
  if (hash.size() < 3) {
    return std::nullopt;
  }
  fs::path file = repo / ".ark" / "objects" / hash.substr(0, 2) / hash.substr(2);
  if (!fs::is_regular_file(file)) {
    return std::nullopt;
  }
  std::string raw = inflate(readFile(file));
  size_t nul = raw.find('\0');
  if (nul == std::string::npos) {
    throw std::runtime_error("object " + hash + " has no header");
  }
  std::string header = raw.substr(0, nul);
  size_t sp = header.find(' ');
  if (sp == std::string::npos) {
    throw std::runtime_error("object " + hash + " has bad header: " + header);
  }
  StoredObject obj{header.substr(0, sp), raw.substr(nul + 1)};
  if (std::stoul(header.substr(sp + 1)) != obj.body.size()) {
    throw std::runtime_error("object " + hash + " size mismatch in header");
  }
  if (sha256Hex(raw) != hash) {
    throw std::runtime_error("object " + hash + " content does not match its id");
  }
  return obj;
}

std::vector<TreeEntry> parseTree(const std::string &body) {
  std::vector<TreeEntry> entries;
  std::istringstream in(body);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream ls(line);
    TreeEntry e;
    ls >> e.mode >> e.type >> e.hash;
    ls.get(); // the single space before the name
    std::getline(ls, e.name);
    entries.push_back(e);
  }
  return entries;
}

CommitInfo parseCommit(const std::string &body) {
  CommitInfo c;
  std::istringstream in(body);
  std::string line;
  std::string message;
  bool inMessage = false;
  while (std::getline(in, line)) {
    if (!inMessage) {
      if (line.rfind("tree ", 0) == 0) {
        c.tree = line.substr(5);
        continue;
      }
      if (line.rfind("parent ", 0) == 0) {
        c.parents.push_back(line.substr(7));
        continue;
      }
      if (line.rfind("committer ", 0) == 0 || line.rfind("author ", 0) == 0) {
        if (line.rfind("committer ", 0) == 0) {
          c.committer = line.substr(10);
        }
        continue;
      }
      inMessage = true;
      if (line.empty()) {
        continue; // blank separator line
      }
    }
    message += line + "\n";
  }
  c.message = trim(message);
  return c;
}

static void flattenInto(const fs::path &repo, const std::string &treeHash,
                        const std::string &prefix, FileMap &out) {
  auto obj = readObject(repo, treeHash);
  if (!obj) {
    throw std::runtime_error("missing tree object " + treeHash);
  }
  if (obj->type != "tree") {
    throw std::runtime_error(treeHash + " is a " + obj->type + ", not a tree");
  }
  for (const auto &e : parseTree(obj->body)) {
    std::string path = prefix.empty() ? e.name : prefix + "/" + e.name;
    if (e.type == "tree") {
      flattenInto(repo, e.hash, path, out);
    } else {
      out[path] = {e.hash, e.mode};
    }
  }
}

FileMap flattenTree(const fs::path &repo, const std::string &treeHash) {
  FileMap out;
  flattenInto(repo, treeHash, "", out);
  return out;
}

FileMap readIndex(const fs::path &repo) {
  FileMap out;
  std::istringstream in(readFile(repo / ".ark" / "index"));
  std::string line;
  while (std::getline(in, line)) {
    if (trim(line).empty()) {
      continue;
    }
    std::istringstream ls(line);
    std::string mode, hash, path;
    ls >> mode >> hash;
    ls.get();
    std::getline(ls, path);
    out[path] = {hash, mode};
  }
  return out;
}

// ---------- snapshots ----------

Snapshot snapshot(const fs::path &root) {
  Snapshot snap;
  for (auto it = fs::recursive_directory_iterator(root);
       it != fs::recursive_directory_iterator(); ++it) {
    const fs::path rel = fs::relative(it->path(), root);
    if (rel == ".ark") {
      it.disable_recursion_pending();
      continue;
    }
    if (rel.filename() == ".ark.log") {
      continue; // covered by its own test (tmp/bugs/07)
    }
    if (it->is_symlink()) {
      FileState st;
      st.symlink = true;
      st.content = fs::read_symlink(it->path()).string();
      snap[rel.generic_string()] = st;
      it.disable_recursion_pending();
    } else if (it->is_regular_file()) {
      FileState st;
      st.content = readFile(it->path());
      st.executable = isExecutable(it->path());
      snap[rel.generic_string()] = st;
    }
  }
  return snap;
}

void PrintTo(const FileState &st, std::ostream *os) {
  *os << (st.symlink ? "symlink->" : "") << '"'
      << (st.content.size() > 40 ? blobHash(st.content).substr(0, 12) + "..." : st.content)
      << '"' << (st.executable ? " (x)" : "");
}

std::string describe(const Snapshot &s) {
  std::ostringstream os;
  for (const auto &[path, st] : s) {
    os << "  " << path << (st.symlink ? " -> " : " : ")
       << (st.content.size() > 40 ? blobHash(st.content).substr(0, 12) + "…"
                                  : st.content)
       << (st.executable ? " (x)" : "") << "\n";
  }
  return os.str();
}

// ---------- status ----------

Status parseStatus(const std::string &output) {
  Status st;
  st.raw = output;
  enum { NONE, STAGED, UNSTAGED, UNTRACKED } section = NONE;
  std::istringstream in(stripAnsi(output));
  std::string line;
  while (std::getline(in, line)) {
    std::string t = trim(line);
    if (t.empty() || t[0] == '[' || t.rfind("Error", 0) == 0) {
      continue; // blank lines, log lines, error lines
    }
    if (t.rfind("Changes to be committed", 0) == 0) {
      section = STAGED;
      continue;
    }
    if (t.rfind("Changes not staged", 0) == 0) {
      section = UNSTAGED;
      continue;
    }
    if (t.rfind("Untracked files", 0) == 0) {
      section = UNTRACKED;
      continue;
    }
    if (section == NONE || t[0] == '(') {
      continue; // "(use ark add ...)" style hints
    }
    // Accept both "path (modified)" and "modified: path" styles.
    bool deleted = false;
    static const std::regex prefix(
        "^(new file|modified|deleted|renamed|typechange):\\s+(.*)$");
    static const std::regex suffix("^(.*?)\\s+\\((modified|untracked|deleted|new)\\)$");
    std::smatch m;
    std::string path = t;
    if (std::regex_match(t, m, prefix)) {
      deleted = m[1] == "deleted";
      path = m[2];
    } else if (std::regex_match(t, m, suffix)) {
      deleted = m[2] == "deleted";
      path = m[1];
    }
    if (deleted) {
      st.deleted.insert(path);
    } else if (section == STAGED) {
      st.staged.insert(path);
    } else if (section == UNSTAGED) {
      st.modified.insert(path);
    } else {
      st.untracked.insert(path);
    }
  }
  return st;
}

// ---------- process runner ----------

std::string RunResult::all() const {
  return "exit=" + std::to_string(exitCode) + "\n--- stdout ---\n" + out +
         "--- stderr ---\n" + err;
}

std::ostream &operator<<(std::ostream &os, const RunResult &r) {
  return os << r.all();
}

static RunResult runProcess(const std::vector<std::string> &argv,
                            const fs::path &cwd,
                            const std::map<std::string, std::string> &env,
                            const fs::path &scratch) {
  static int counter = 0;
  const fs::path outFile = scratch / ("stdout-" + std::to_string(++counter));
  const fs::path errFile = scratch / ("stderr-" + std::to_string(counter));

  std::vector<std::string> envStrings;
  for (const auto &[k, v] : env) {
    envStrings.push_back(k + "=" + v);
  }
  std::vector<char *> envp;
  for (auto &s : envStrings) {
    envp.push_back(s.data());
  }
  envp.push_back(nullptr);
  std::vector<std::string> argStrings(argv);
  std::vector<char *> args;
  for (auto &s : argStrings) {
    args.push_back(s.data());
  }
  args.push_back(nullptr);

  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("fork failed");
  }
  if (pid == 0) {
    if (chdir(cwd.c_str()) != 0) {
      _exit(126);
    }
    int in = open("/dev/null", O_RDONLY);
    int out = open(outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int err = open(errFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (in < 0 || out < 0 || err < 0) {
      _exit(126);
    }
    dup2(in, 0);
    dup2(out, 1);
    dup2(err, 2);
    execve(args[0], args.data(), envp.data());
    _exit(127);
  }

  // Wait with a timeout so a stuck command (e.g. an editor waiting for
  // input) fails the test instead of hanging the whole suite.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  int status = 0;
  bool timedOut = false;
  while (true) {
    pid_t r = waitpid(pid, &status, WNOHANG);
    if (r == pid) {
      break;
    }
    if (std::chrono::steady_clock::now() > deadline) {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      timedOut = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  RunResult r;
  r.out = readFile(outFile);
  r.err = readFile(errFile);
  if (timedOut) {
    r.exitCode = -2;
    r.err += "\n[ark-test] TIMED OUT after 30s";
  } else if (WIFEXITED(status)) {
    r.exitCode = WEXITSTATUS(status);
  } else {
    r.exitCode = 128 + WTERMSIG(status); // crashed: SIGSEGV -> 139, abort -> 134
  }
  return r;
}

Sandbox::Sandbox() {
  fs::create_directories(repo());
  fs::create_directories(home());
  fs::create_directories(root() / "tmp");
  fs::create_directories(root() / "io");
  // A non-interactive $GIT_EDITOR: writes $ARK_TEST_MSG into the file it is given.
  editor_ = root() / "editor.sh";
  writeFile(editor_, "#!/bin/sh\nprintf '%s' \"$ARK_TEST_MSG\" > \"$1\"\n");
  setExecutable(editor_, true);
}

RunResult Sandbox::ark(const std::vector<std::string> &args, const fs::path &cwd,
                       const std::map<std::string, std::string> &extraEnv) {
  std::map<std::string, std::string> env = {
      {"PATH", "/usr/bin:/bin:/usr/sbin:/sbin"},
      {"HOME", home().string()},
      {"TMPDIR", (root() / "tmp").string() + "/"},
      {"GIT_EDITOR", editor_.string()},
      {"LANG", "C"},
  };
  for (const auto &[k, v] : extraEnv) {
    env[k] = v;
  }
  std::vector<std::string> argv{ARK_BIN};
  argv.insert(argv.end(), args.begin(), args.end());
  RunResult r = runProcess(argv, cwd.empty() ? repo() : cwd, env, root() / "io");
  // In an ARK_SANITIZE build, memory errors and UB in the binary are test failures
  // even when the command happens to produce the right output.
  if (r.err.find("ERROR: AddressSanitizer") != std::string::npos ||
      r.err.find("runtime error:") != std::string::npos) {
    std::string cmd = "ark";
    for (const auto &a : args) {
      cmd += " " + a;
    }
    ADD_FAILURE() << "sanitizer report while running `" << cmd << "`\n" << r.err;
  }
  return r;
}

void Sandbox::initRepo(bool withIdentity) {
  RunResult r = ark({"init"});
  ASSERT_EQ(r.exitCode, 0) << r;
  ASSERT_TRUE(fs::is_directory(repo() / ".ark")) << r;
  if (withIdentity) {
    setRepoIdentity("Test User", "test@example.com");
  }
}

void Sandbox::setRepoIdentity(const std::string &name, const std::string &email) {
  writeFile(repo() / ".ark" / "config",
            "[user]\n\tname = " + name + "\n\temail = " + email + "\n");
}

RunResult Sandbox::commit(const std::string &message) {
  return ark({"commit"}, {}, {{"ARK_TEST_MSG", message}});
}

std::string Sandbox::addAndCommit(const std::vector<std::string> &paths,
                                  const std::string &message) {
  std::vector<std::string> args{"add"};
  args.insert(args.end(), paths.begin(), paths.end());
  RunResult add = ark(args);
  EXPECT_EQ(add.exitCode, 0) << add;
  const std::string before = headCommit();
  RunResult c = commit(message);
  EXPECT_EQ(c.exitCode, 0) << c;
  const std::string after = headCommit();
  EXPECT_NE(after, before) << "HEAD did not move after commit\n" << c;
  return after;
}

void Sandbox::write(const std::string &rel, const std::string &content) {
  writeFile(repo() / rel, content);
}

std::string Sandbox::read(const std::string &rel) const {
  return readFile(repo() / rel);
}

bool Sandbox::exists(const std::string &rel) const {
  return fs::exists(fs::symlink_status(repo() / rel));
}

void Sandbox::remove(const std::string &rel) { fs::remove_all(repo() / rel); }

std::string Sandbox::headFile() const {
  return trim(readFile(repo() / ".ark" / "HEAD"));
}

std::string Sandbox::branchCommit(const std::string &branch) const {
  fs::path p = repo() / ".ark" / "refs" / "heads" / branch;
  if (!fs::exists(p)) {
    return "";
  }
  std::string h = trim(readFile(p));
  return h == std::string(64, '0') ? "" : h;
}

std::string Sandbox::headCommit() const {
  std::string head = headFile();
  if (head.rfind("ref: refs/heads/", 0) == 0) {
    return branchCommit(head.substr(16));
  }
  return head;
}

std::optional<StoredObject> Sandbox::object(const std::string &hash) const {
  return readObject(repo(), hash);
}

CommitInfo Sandbox::commitInfo(const std::string &hash) const {
  auto obj = object(hash);
  if (!obj) {
    throw std::runtime_error("missing commit object " + hash);
  }
  if (obj->type != "commit") {
    throw std::runtime_error(hash + " is a " + obj->type + ", not a commit");
  }
  return parseCommit(obj->body);
}

FileMap Sandbox::commitFiles(const std::string &commitHash) const {
  return flattenTree(repo(), commitInfo(commitHash).tree);
}

Status Sandbox::status(const fs::path &cwd) {
  RunResult r = ark({"status"}, cwd);
  EXPECT_EQ(r.exitCode, 0) << r;
  return parseStatus(r.out);
}

} // namespace arktest
