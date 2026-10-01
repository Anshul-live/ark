// merge feature work begins
#include <ark/cli/commands.h>
#include <ark/cli/commit_message_editor.h>
#include <ark/repo/index.h>
#include <ark/objects/objects.h>
#include <ark/repo/ref_store.h>

int cmd_merge(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: ark merge <branch-name>\n";
    return 1;
  }

  const std::string &branchName = args[0];
  RefStore ref;

  const auto branch = ark::BranchName::unchecked(branchName);

  if (!ref.branchExists(branch)) {
    std::cerr << "branch " << branchName << " does not exist\n";
    return 1;
  }

  auto ourHead = ref.resolveHead();
  std::string ourCommitHash = ourHead ? ourHead->hex() : "";
  auto theirHead = ref.branchTarget(branch);
  std::string theirCommitHash = theirHead ? theirHead->hex() : "";
  auto ourBranch = ref.currentBranch();
  std::string ourBranchName = ourBranch ? ourBranch->str() : "";

  if (!ref.isHeadDetached() && ourBranchName == branchName) {
    std::cerr << "their branch is same as ours" << "\n";
    return 0;
  }

  const std::vector<std::string> temp = {ourCommitHash};

  cmd_catFile(args);

  // fast forward merge

  return 0;
}
