#include <ark/objects/tree_diff.h>

namespace ark {

std::vector<FileChange> diffTrees(const FlatTree &from, const FlatTree &to) {
  std::vector<FileChange> changes;
  auto a = from.begin();
  auto b = to.begin();
  while (a != from.end() || b != to.end()) {
    if (b == to.end() || (a != from.end() && a->first < b->first)) {
      changes.push_back({ChangeKind::Deleted, a->first, a->second, std::nullopt});
      ++a;
    } else if (a == from.end() || b->first < a->first) {
      changes.push_back({ChangeKind::Added, b->first, std::nullopt, b->second});
      ++b;
    } else {
      if (a->second.hash != b->second.hash)
        changes.push_back({ChangeKind::Modified, a->first, a->second, b->second});
      ++a;
      ++b;
    }
  }
  return changes;
}

} // namespace ark
