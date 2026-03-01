// merge feature work begins
#include "cat-file.h"
#include <ark.h>
#include <index.h>
#include <merge.h>
#include <objects.h>
#include <ref.h>

int cmd_merge(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: ark merge <branch-name>\n";
    return 1;
  }

  const std::string &branchName = args[0];
  Ref ref;

  if (!ref.branchExists(branchName)) {
    std::cerr << "branch " << branchName << " does not exist\n";
    return 1;
  }

  std::string ourCommitHash = ref.getHeadCommit();
  std::string theirCommitHash = ref.getBranchHash(branchName);
  std::string ourBranchName = ref.getCurrentBranchName();

  if (!ref.isDetached() && ourBranchName == branchName) {
    std::cerr << "their branch is same as ours" << "\n";
    return 0;
  }

  const std::vector<std::string> temp = {ourCommitHash};

  cmd_catFile(args);

  // fast forward merge

  return 0;
}
