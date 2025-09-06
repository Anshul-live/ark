#include <iostream>
#include <ark.h>
#include <filesystem>
#include <fstream>
#include <switch.h>

void switchBranch(const std::string& branch_name){
//TODO: implement logic of updating working dir based on branch 
  std::string repo_root = arkDir();
  std::string branch_path = repo_root + "/.ark/refs/heads/" + branch_name;
  if(!(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path))){
    std::cerr<<"branch" << branch_name <<" does not exist\n";
    return;
  }
  std::ofstream(repo_root+"/.ark/HEAD") << "ref: refs/heads/" + branch_name;
  std::cout<<"switched branch to "<<branch_name<<"\n";
}
