#include <ark/core/repo_path.h>

namespace ark {

namespace {

std::vector<std::string> splitComponents(std::string_view path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    std::size_t end = path.find('/', start);
    if (end == std::string_view::npos)
      end = path.size();
    std::string_view part = path.substr(start, end - start);
    if (part.empty() || part == ".") {
    } else if (part == "..") {
      if (!parts.empty() && parts.back() != "..")
        parts.pop_back();
      else
        parts.emplace_back("..");
    } else {
      parts.emplace_back(part);
    }
    start = end + 1;
  }
  return parts;
}

} // namespace

RepoPath RepoPath::fromRelative(std::string_view path) {
  std::string out;
  for (const auto &part : splitComponents(path)) {
    if (!out.empty())
      out += '/';
    out += part;
  }
  return RepoPath(std::move(out));
}

std::optional<RepoPath>
RepoPath::fromAbsolute(const std::filesystem::path &absolute,
                       const std::filesystem::path &root) {
  std::filesystem::path rel =
      absolute.lexically_normal().lexically_relative(root.lexically_normal());
  if (rel.empty())
    return std::nullopt;
  std::string text = rel.generic_string();
  if (text == ".." || text.compare(0, 3, "../") == 0)
    return std::nullopt;
  return fromRelative(text);
}

RepoPath RepoPath::child(std::string_view name) const {
  if (path_.empty())
    return fromRelative(name);
  return fromRelative(path_ + "/" + std::string(name));
}

std::vector<std::string> RepoPath::components() const {
  return splitComponents(path_);
}

std::filesystem::path
RepoPath::toAbsolute(const std::filesystem::path &root) const {
  return path_.empty() ? root : root / path_;
}

bool RepoPath::isInsideArkDir() const {
  return path_ == ".ark" || path_.compare(0, 5, ".ark/") == 0;
}

std::ostream &operator<<(std::ostream &os, const RepoPath &path) {
  return os << path.str();
}

} // namespace ark
