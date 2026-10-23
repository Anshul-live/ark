// Not compiled yet: merge support is tracked in GitHub issues #10 to #14.
#include <algorithm>
#include <ark/cli/commands.h>
#include <ark/objects/commit_dag.h>
#include <ark/objects/commit_store.h>
#include <ark/repo/ref_store.h>
#include <ark/storage/object_store.h>
#include <iostream>

int cmd_merge(const Repository &repo, const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: ark merge <branch-name>\n";
    return 1;
  }

  const std::string &branchName = args[0];
  RefStore ref(repo);

  const auto branch = ark::BranchName::unchecked(branchName);

  if (!ref.branchExists(branch)) {
    std::cerr << "branch " << branchName << " does not exist\n";
    return 1;
  }

  auto ourHead = ref.resolveHead();
  auto theirHead = ref.branchTarget(branch);
  auto ourBranch = ref.currentBranch();
  std::string ourBranchName = ourBranch ? ourBranch->str() : "";

  if (!ref.isHeadDetached() && ourBranchName == branchName) {
    std::cerr << "their branch is same as ours" << "\n";
    return 0;
  }

  ark::ObjectStore store(repo.objectsDir());
  ark::CommitStore commits(store);
  ark::CommitDAG dag(commits);

  std::vector<ark::ObjectHash> ourHistory = dag.historyFrom(*ourHead);
  std::vector<ark::ObjectHash> theirHistory = dag.historyFrom(*theirHead);

  auto contains = [](const std::vector<ark::ObjectHash> &history,
                     const ark::ObjectHash &hash) {
    return std::find(history.begin(), history.end(), hash) != history.end();
  };

  if (contains(ourHistory, *theirHead)) {
    std::cout << "already up to date\n";
    return 0;
  }

  if (contains(theirHistory, *ourHead)) {
    // fast forward merge
    ref.moveHeadTo(*theirHead);
    return 0;
  }

  // three-way merge
  return 0;
}
