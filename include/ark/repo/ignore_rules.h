#ifndef ARK_REPO_IGNORE_RULES_H
#define ARK_REPO_IGNORE_RULES_H

#include <filesystem>
#include <string>
#include <unordered_set>

class Repository;

namespace ark {

class IgnoreRules {
public:
  // Collects the patterns of every .arkignore file below the repository root.
  static IgnoreRules load(const Repository &repo);

  bool matches(const std::filesystem::path &absolute, bool isDirectory) const;

private:
  bool matchesNormalized(const std::string &normalized) const;

  std::unordered_set<std::string> patterns_;
};

} // namespace ark

#endif
