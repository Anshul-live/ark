#include <ark/cli/commit_message_editor.h>
#include <filesystem>
#include <fstream>
#include <ark/repo/index.h>
#include <iostream>
#include <ark/objects/tree_diff.h>
#include <ark/repo/checkout.h>
#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/objects/flat_tree.h>
#include <ark/storage/object_store.h>
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

  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  ark::CommitStore commits(store);
  auto filesOf = [&](const std::string &hash) {
    if (hash == NULL_HASH)
      return ark::FlatTree{};
    ark::Commit commit = commits.read(ark::ObjectHash::unchecked(hash));
    return ark::flattenTree(store, commit.tree);
  };

  ark::FlatTree sourceFiles;
  if (!sourceCommitHash.empty()) {
    sourceFiles = filesOf(sourceCommitHash);
  }

  ark::FlatTree targetFiles = filesOf(targetCommitHash);
  std::vector<ark::FileChange> changes =
      ark::diffTrees(sourceFiles, targetFiles);

  ref.attachHeadTo(branch);
  ark::Checkout(repo, store).apply(changes);

  Index idx;
  idx.clear();

  idx.replaceAll(targetFiles);

  idx.save();

  std::cout << "switched branch to " << branchName << "\n";
  return 0;
}
