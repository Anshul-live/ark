#ifndef ARK_OBJECTS_TREE_DIFF_H
#define ARK_OBJECTS_TREE_DIFF_H

#include <ark/core/repo_path.h>
#include <ark/objects/file_entry.h>
#include <ark/objects/flat_tree.h>

#include <optional>
#include <vector>

namespace ark {

enum class ChangeKind { Added, Deleted, Modified };

struct FileChange {
  ChangeKind kind;
  RepoPath path;
  std::optional<FileEntry> before;
  std::optional<FileEntry> after;
};

// Files are compared by hash only; a mode-only change is not reported.
// The result is sorted by path.
std::vector<FileChange> diffTrees(const FlatTree &from, const FlatTree &to);

} // namespace ark

#endif
