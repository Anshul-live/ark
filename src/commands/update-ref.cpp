#include <update-ref.h>
#include <ark.h>

int cmd_updateRef(const std::vector<std::string> &args){
  if(args.size() < 2){
            std::cout << "Usage: update-ref <ref-path> <commit-hash>" << std::endl;
            return 1;
  }
  updateRef(args[0],args[1]);
  return 0;
}

void updateRef(const std::string& ref_path,const std::string& hash){
  std::string repo_root = arkDir();
  if(ref_path.rfind("refs/heads/",0) != 0){
    std::cerr<<"provided ref_path is not a branch\n";
    return;
  }
  if(!std::filesystem::exists(repo_root+"/.ark/"+ref_path)){
    std::cerr<<"branch "<<ref_path<<"does not exist\n";
    return;
  }
  std::ofstream out(repo_root+"/.ark/"+ref_path,std::ios::binary);
  if(!out){
    std::cerr<<"error opening file"<<ref_path<<std::endl;
  }
  out << hash;
}
