#ifndef ARK_CORE_REPO_PATH_H
#define ARK_CORE_REPO_PATH_H

#include <filesystem>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ark {

// A repository-relative path: '/'-separated and lexically normal, with no
// leading "./", no trailing '/', and no "." or ".." components.
class RepoPath {
public:
  RepoPath() = default;

  static RepoPath fromRelative(std::string_view path);
  static std::optional<RepoPath>
  fromAbsolute(const std::filesystem::path &absolute,
               const std::filesystem::path &root);

  const std::string &str() const { return path_; }
  bool empty() const { return path_.empty(); }
  RepoPath child(std::string_view name) const;
  std::vector<std::string> components() const;
  std::filesystem::path toAbsolute(const std::filesystem::path &root) const;
  bool isInsideArkDir() const;

  friend bool operator==(const RepoPath &a, const RepoPath &b) {
    return a.path_ == b.path_;
  }
  friend bool operator!=(const RepoPath &a, const RepoPath &b) {
    return a.path_ != b.path_;
  }
  friend bool operator<(const RepoPath &a, const RepoPath &b) {
    return a.path_ < b.path_;
  }

private:
  explicit RepoPath(std::string path) : path_(std::move(path)) {}
  std::string path_;
};

std::ostream &operator<<(std::ostream &os, const RepoPath &path);

} // namespace ark

namespace std {
template <> struct hash<ark::RepoPath> {
  size_t operator()(const ark::RepoPath &p) const noexcept {
    return std::hash<std::string>{}(p.str());
  }
};
} // namespace std

#endif
