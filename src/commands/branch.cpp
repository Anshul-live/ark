#include <branch.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark.h>
#include <ref.h>

int cmd_branch(const std::vector<std::string> &args){
  Ref ref;
  
  if(args.size() < 1){
      auto branches = ref.listBranches();
      std::string currentBranch = ref.getCurrentBranchName();
      for (const auto& branch : branches) {
          if (branch == currentBranch) {
              std::cout << "* \033[32m" << branch << "\033[0m\n";
          } else {
              std::cout << "  " << branch << "\n";
          }
      }
      return 0;
  }
  
  std::string name = args[0];
  std::string commitHash = ref.getHeadCommit();
  
  if(commitHash.empty() || commitHash == NULL_HASH){
    std::cerr << "please make a commit first";
    return 1;
  }
  
  if(ref.branchExists(name)){
    std::cout << "branch already exists" << std::endl;
    return 1;
  }
  
  ref.createBranch(name, commitHash);
  return 0;
}
