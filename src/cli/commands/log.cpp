#include <ark/cli/commands.h>
#include <ark/objects/commit_dag.h>
#include <ark/objects/commit_store.h>
#include <ark/repo/ref_store.h>
#include <ark/storage/object_store.h>
#include <iostream>
#include <string>

namespace {

void printHistory(const std::vector<ark::ObjectHash> &history) {
  for (const ark::ObjectHash &hash : history)
    std::cout << "\033[1;33mcommit: " << hash.hex() << "\033[0m\n";
}

} // namespace

int cmd_log(const Repository &repo, const std::vector<std::string> &args) {
  RefStore ref(repo);
  auto branch = ref.currentBranch();

  if (!branch) {
    std::cout << "HEAD is detached\n";
    return 1;
  }

  auto target = ref.branchTarget(*branch);
  ark::ObjectHash start = target ? *target : ark::ObjectHash::unchecked("");
  ark::ObjectStore store(repo.objectsDir());
  ark::CommitStore commits(store);
  printHistory(ark::CommitDAG(commits).historyFrom(start));
  return 0;
}
