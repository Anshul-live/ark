#ifndef ARK_REPO_WORKING_TREE_H
#define ARK_REPO_WORKING_TREE_H

#include <ark/objects/flat_tree.h>
#include <ark/repo/ignore_rules.h>
#include <ark/repo/repository.h>

#include <filesystem>
#include <utility>
#include <vector>

namespace ark {

class WorkingTree {
public:
  WorkingTree(const Repository &repo, const IgnoreRules &ignore)
      : repo_(repo), ignore_(ignore) {}

  // Hash and mode of every non-ignored regular file under the root.
  FlatTree scan() const;

  // Calls visit(absolutePath) for each non-ignored regular file under start,
  // breadth first. Stops and returns false as soon as visit returns false.
  template <class Visit>
  bool forEachFile(const std::filesystem::path &start, Visit visit) const {
    return forEachFile(start, std::move(visit),
                       [](const std::filesystem::path &) {});
  }

  // As above; onMissing(path) is called for a non-ignored path that does not
  // exist.
  template <class Visit, class Missing>
  bool forEachFile(const std::filesystem::path &start, Visit visit,
                   Missing onMissing) const {
    std::vector<std::filesystem::path> pending{start};
    for (size_t i = 0; i < pending.size(); i++) {
      std::filesystem::path path = std::filesystem::absolute(pending[i]);

      if (ignore_.matches(path, false)) {
        continue;
      }
      if (!std::filesystem::exists(path)) {
        onMissing(path);
        continue;
      }
      if (std::filesystem::is_regular_file(path)) {
        if (!visit(path))
          return false;
      } else if (std::filesystem::is_directory(path)) {
        if (ignore_.matches(path, true)) {
          continue;
        }
        for (const auto &entry : std::filesystem::directory_iterator(path)) {
          pending.push_back(entry.path());
        }
      }
    }
    return true;
  }

private:
  const Repository &repo_;
  const IgnoreRules &ignore_;
};

} // namespace ark

#endif
