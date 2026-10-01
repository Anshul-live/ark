#include <ark/cli/commit_message_editor.h>
#include <filesystem>
#include <fstream>
#include <ark/repo/index.h>
#include <iostream>
#include <ark/objects/objects.h>
#include <ark/repo/ref_store.h>
#include <ark/repo/repository.h>
#include <ark/cli/commands.h>

int cmd_switch(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: ark switch <branch-name>\n";
    return 1;
  }

  const std::string &branchName = args[0];
  RefStore ref;

  const auto branch = ark::BranchName::unchecked(branchName);

  if (!ref.branchExists(branch)) {
    std::cerr << "branch " << branchName << " does not exist\n";
    return 1;
  }

  auto sourceHead = ref.resolveHead();
  std::string sourceCommitHash = sourceHead ? sourceHead->hex() : "";
  std::string targetCommitHash = ref.branchTarget(branch)->hex();
  auto sourceBranch = ref.currentBranch();
  std::string sourceBranchName = sourceBranch ? sourceBranch->str() : "";

  if (!ref.isHeadDetached() && sourceBranchName == branchName) {
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

  ref.attachHeadTo(branch);
  Tree::buildFromDiff(diff);

  Index idx;
  idx.clear();

  auto flat = targetCommit.tree->flatten();
  ark::FlatTree newEntries;
  for (auto &[path, pair] : flat) {
    ark::FileEntry entry{
        ark::ObjectHash::unchecked(pair.first),
        ark::parseFileMode(pair.second).value_or(ark::FileMode::Regular)};
    newEntries.insert_or_assign(ark::RepoPath::fromRelative(path), entry);
  }
  idx.replaceAll(std::move(newEntries));

  idx.save();

  std::cout << "switched branch to " << branchName << "\n";
  return 0;
}
