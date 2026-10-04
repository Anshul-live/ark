#include <ark/cli/commit_message_editor.h>
#include <ark/cli/commands.h>
#include <ark/util/strings.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/repo/repository.h>
#include <ark/storage/object_store.h>
#include <queue>
#include <ark/repo/ref_store.h>
#include <sstream>
#include <string>

void logBranch(const std::string &branchName) {
  RefStore ref;
  auto target = ref.branchTarget(ark::BranchName::unchecked(branchName));
  std::string commitHash = target ? target->hex() : "";
  std::queue<std::string> commit_hashes_queue;
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  ark::CommitStore commits(store);

  while (commitHash != NULL_HASH) {
    std::cout << "\033[1;33mcommit: " << commitHash << "\033[0m\n";
    ark::Commit c = commits.read(ark::ObjectHash::unchecked(commitHash));
    for (const auto &parent : c.parents)
      commit_hashes_queue.push(parent.hex());
    if (commit_hashes_queue.empty())
      break;
    commitHash = commit_hashes_queue.front();
    commit_hashes_queue.pop();
  }
}

int cmd_log(const std::vector<std::string> &args) {
  RefStore ref;
  auto branch = ref.currentBranch();

  if (!branch) {
    std::cout << "HEAD is detached\n";
    return 1;
  }

  logBranch(branch->str());
  return 0;
}
