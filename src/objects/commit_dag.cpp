#include <ark/objects/commit_dag.h>

#include <queue>

namespace ark {

std::vector<ObjectHash> CommitDAG::historyFrom(const ObjectHash &start) const {
  std::vector<ObjectHash> history;
  std::queue<ObjectHash> pending;
  ObjectHash current = start;

  while (current != ObjectHash::zero()) {
    history.push_back(current);
    Commit c = commits_.read(current);
    for (const ObjectHash &parent : c.parents)
      pending.push(parent);
    if (pending.empty())
      break;
    current = pending.front();
    pending.pop();
  }
  return history;
}

} // namespace ark
