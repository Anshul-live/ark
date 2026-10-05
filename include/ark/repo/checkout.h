#ifndef ARK_REPO_CHECKOUT_H
#define ARK_REPO_CHECKOUT_H

#include <ark/objects/tree_diff.h>

#include <string>
#include <vector>

class Repository;

namespace ark {

class ObjectStore;

class Checkout {
public:
  Checkout(const Repository &repo, const ObjectStore &store);

  // Deletions first, then creations, then modifications.
  void apply(const std::vector<FileChange> &changes);

private:
  bool createFile(const std::string &path, FileMode mode,
                  const std::string &data);
  bool removeFile(const std::string &path);
  bool overwriteFile(const std::string &path, FileMode mode,
                     const std::string &data);
  std::string fullPath(const RepoPath &path) const;
  std::string blobData(const ObjectHash &hash) const;

  std::string root_;
  const ObjectStore &store_;
};

} // namespace ark

#endif
