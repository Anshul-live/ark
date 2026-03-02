#include <ark.h>
#include <filesystem>
#include <fstream>
#include <index.h>
#include <iostream>
#include <objects.h>
#include <ref.h>
#include <repository.h>
#include <switch.h>

int cmd_switch(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: ark switch <branch-name>\n";
    return 1;
  }

  const std::string &branchName = args[0];
  Ref ref;

  if (!ref.branchExists(branchName)) {
    std::cerr << "branch " << branchName << " does not exist\n";
    return 1;
  }

  std::string sourceCommitHash = ref.getHeadCommit();
  std::string targetCommitHash = ref.getBranchHash(branchName);
  std::string sourceBranchName = ref.getCurrentBranchName();

  if (!ref.isDetached() && sourceBranchName == branchName) {
    std::cerr << "already on branch " << branchName << "\n";
    return 0;
  }

  Commit sourceCommit;
  if (!sourceCommitHash.empty()) {
    sourceCommit.loadFromDisk(sourceCommitHash);
  }

  Commit targetCommit;
  targetCommit.loadFromDisk(targetCommitHash);
  std::unordered_map<std::string, std::vector<std::pair<Object *, std::string>>>
      diff;
  Repository repo;
  Tree::diff(sourceCommit.tree->root, targetCommit.tree->root, diff,
             repo.root());

  ref.setHeadToBranch(branchName);
  Tree::buildFromDiff(diff);

  Index idx;
  idx.clear();

  auto flat = targetCommit.tree->flatten();
  std::unordered_map<std::string, IndexEntry> newEntries;
  for (auto &[path, pair] : flat) {
    IndexEntry entry;
    entry.hash = pair.first;
    entry.mode = pair.second;
    newEntries[path] = entry;
  }
  idx.setAll(newEntries);

  idx.save();

  std::cout << "switched branch to " << branchName << "\n";
  return 0;
}
