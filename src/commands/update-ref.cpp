#include <update-ref.h>
#include <ark.h>
#include <ref.h>

int cmd_updateRef(const std::vector<std::string> &args){
  if(args.size() < 2){
            std::cout << "Usage: update-ref <ref-path> <commit-hash>" << std::endl;
            return 1;
  }
  updateRef(args[0], args[1]);
  return 0;
}

void updateRef(const std::string& ref_path,const std::string& hash){
  Ref ref;
  if(ref_path.rfind("refs/heads/",0) != 0){
    std::cerr << "provided ref_path is not a branch\n";
    return;
  }
  
  std::string branchName = ref_path.substr(11);
  if(!ref.branchExists(branchName)){
    std::cerr << "branch " << ref_path << " does not exist\n";
    return;
  }
  
  ref.updateBranch(branchName, hash);
}
