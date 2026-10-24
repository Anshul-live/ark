#include <ark/repo/ignore_rules.h>

#include <ark/core/exceptions.h>
#include <ark/repo/repository.h>
#include <ark/util/fs.h>
#include <ark/util/logger.h>
#include <ark/util/profiler.h>
#include <ark/util/strings.h>

#include <fstream>
#include <vector>

namespace ark {

IgnoreRules IgnoreRules::load(const Repository &repo) {
  ARK_PROF_SCOPE("ignore.load");
  IgnoreRules rules;
  std::vector<std::string> paths;

  try {
    paths.push_back(repo.root());
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("IgnoreRules::load", e);
    return rules;
  }

  for (size_t i = 0; i < paths.size(); i++) {
    std::string path = paths[i];
    std::string ignoreFile = path + "/.arkignore";

    if (std::filesystem::exists(ignoreFile) &&
        std::filesystem::is_regular_file(ignoreFile)) {
      std::ifstream in(ignoreFile);
      std::string line;
      while (std::getline(in, line)) {
        line = text::trim(line);
        if (line.empty() || line[0] == '#') {
          continue;
        }
        rules.patterns_.insert(normalizeGenericPath(path + "/" + line));
      }
    }

    try {
      ARK_PROF_COUNT("ignore.dirs_listed", 1);
      for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_directory(entry)) {
          paths.push_back(entry.path().string());
        }
      }
    } catch (const std::exception &e) {
      LOG_WARNING("Error reading directory: " + path);
    }
  }
  return rules;
}

bool IgnoreRules::matchesNormalized(const std::string &normalized) const {
  for (const auto &pattern : patterns_) {
    if (normalized == pattern) {
      return true;
    }
    if (!pattern.empty() && pattern.back() == '/') {
      if (normalized.rfind(pattern, 0) == 0) {
        return true;
      }
    }
  }
  return false;
}

bool IgnoreRules::matches(const std::filesystem::path &absolute,
                          bool isDirectory) const {
  const std::string path = absolute.string();
  if (matchesNormalized(normalizeGenericPath(path)))
    return true;
  return isDirectory && matchesNormalized(normalizeGenericPath(path + "/"));
}

} // namespace ark
