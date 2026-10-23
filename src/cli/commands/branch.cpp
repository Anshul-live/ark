#include <ark/cli/commands.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark/cli/commit_message_editor.h>
#include <ark/repo/ref_store.h>

int cmd_branch(const Repository &repo, const std::vector<std::string> &args){
  RefStore ref(repo);
  
  if(args.size() < 1){
      auto branches = ref.listBranches();
      auto current = ref.currentBranch();
      std::string currentBranch = current ? current->str() : "";
      for (const auto& branch : branches) {
          if (branch.str() == currentBranch) {
              std::cout << "* \033[32m" << branch.str() << "\033[0m\n";
          } else {
              std::cout << "  " << branch.str() << "\n";
          }
      }
      return 0;
  }
  
  std::string name = args[0];
  auto head = ref.resolveHead();
  
  if(!head || head->isZero()){
    std::cerr << "please make a commit first";
    return 1;
  }
  
  const auto branch = ark::BranchName::unchecked(name);
  if(ref.branchExists(branch)){
    std::cout << "branch already exists" << std::endl;
    return 1;
  }
  
  ref.createBranch(branch, *head);
  return 0;
}
