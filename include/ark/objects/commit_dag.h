#ifndef ARK_OBJECTS_COMMIT_DAG_H
#define ARK_OBJECTS_COMMIT_DAG_H

#include <ark/core/object_hash.h>
#include <ark/objects/commit_store.h>

#include <vector>

namespace ark {

class CommitDAG {
public:
  explicit CommitDAG(const CommitStore &commits) : commits_(commits) {}

  // Breadth-first walk from start, first parents before later ones. A commit
  // reachable along several paths is listed once per path.
  std::vector<ObjectHash> historyFrom(const ObjectHash &start) const;

private:
  const CommitStore &commits_;
};

} // namespace ark

#endif
