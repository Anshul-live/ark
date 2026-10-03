#include <ark/cli/commit_message_editor.h>
#include <filesystem>
#include <fstream>
#include <ark/repo/index.h>
#include <iostream>
#include <ark/objects/checkout_legacy.h>
#include <ark/objects/objects.h>
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

  Commit sourceCommit;
  if (!sourceCommitHash.empty()) {
    sourceCommit.loadFromDisk(sourceCommitHash);
  }

  Commit targetCommit;
  targetCommit.loadFromDisk(targetCommitHash);
  Repository repo;
  ark::CheckoutPlan plan =
      ark::planCheckout(sourceCommit.files, targetCommit.files, repo.root());

  ref.attachHeadTo(branch);
  ark::ObjectStore store(repo.objectsDir());
  ark::applyCheckout(plan, store);

  Index idx;
  idx.clear();

  idx.replaceAll(targetCommit.files);

  idx.save();

  std::cout << "switched branch to " << branchName << "\n";
  return 0;
}
