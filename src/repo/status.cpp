#include <ark/repo/status.h>

#include <ark/repo/index.h>

#include <set>

namespace ark {

StatusReport computeStatus(const Index &index, const FlatTree &headTree,
                           const FlatTree &worktree) {
  StatusReport report;
  std::set<RepoPath> staged;

  for (const auto &[path, entry] : index.entries()) {
    auto it = headTree.find(path);
    if (it == headTree.end() || it->second.hash != entry.hash) {
      report.staged.push_back(path);
      staged.insert(path);
    }
  }

  for (const auto &[path, entry] : worktree) {
    const FileEntry *indexed = index.find(path);
    if (indexed && indexed->hash != entry.hash && !staged.count(path))
      report.modified.push_back(path);
  }

  for (const auto &[path, entry] : worktree) {
    if (!index.contains(path) && headTree.find(path) == headTree.end() &&
        !staged.count(path))
      report.untracked.push_back(path);
  }
  return report;
}

void printStatus(const StatusReport &report, std::ostream &out) {
  out << "Changes to be committed:\n";
  for (const RepoPath &path : report.staged)
    out << "\033[32m" << path.str() << "\033[0m\n";

  out << "\nChanges not staged for commit:\n";
  for (const RepoPath &path : report.modified)
    out << "\033[31m" << path.str() << " (modified)\033[0m\n";

  out << "\nUntracked files:\n";
  for (const RepoPath &path : report.untracked)
    out << "\033[31m" << path.str() << " (untracked)\033[0m\n";
}

} // namespace ark
