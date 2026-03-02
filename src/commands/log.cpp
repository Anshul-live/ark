#include <ark.h>
#include <cat-file.h>
#include <config.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <log.h>
#include <objects.h>
#include <queue>
#include <ref.h>
#include <sstream>
#include <string>

void logBranch(const std::string &branchName) {
  Ref ref;
  Config config;
  std::string commitHash = ref.getBranchHash(branchName);
  std::queue<std::string> commit_hashes_queue;

  while (commitHash != NULL_HASH) {
    std::string content = Object::readFromDisk(commitHash);
    std::vector<std::string> lines = config.split(content, '\n');
    std::vector<std::string> data;
    std::cout << "\033[1;33mcommit: " << commitHash << "\033[0m\n";
    Commit c;
    c.loadFromDisk(commitHash);
    for (auto parent : c.parents)
      commit_hashes_queue.push(parent);
    if (commit_hashes_queue.empty())
      break;
    commitHash = commit_hashes_queue.front();
    commit_hashes_queue.pop();
  }
}

int cmd_log(const std::vector<std::string> &args) {
  Ref ref;
  std::string currentBranch = ref.getCurrentBranchName();

  if (currentBranch.empty()) {
    std::cout << "HEAD is detached\n";
    return 1;
  }

  logBranch(currentBranch);
  return 0;
}
