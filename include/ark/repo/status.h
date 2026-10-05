#ifndef ARK_REPO_STATUS_H
#define ARK_REPO_STATUS_H

#include <ark/core/repo_path.h>
#include <ark/objects/flat_tree.h>

#include <ostream>
#include <vector>

class Index;

namespace ark {

struct StatusReport {
  std::vector<RepoPath> staged;
  std::vector<RepoPath> modified;
  std::vector<RepoPath> untracked;

  bool isClean() const {
    return staged.empty() && modified.empty() && untracked.empty();
  }
};

StatusReport computeStatus(const Index &index, const FlatTree &headTree,
                           const FlatTree &worktree);
void printStatus(const StatusReport &report, std::ostream &out);

} // namespace ark

#endif
