#include <iostream>
#include <ark.h>
#include <filesystem>
#include <fstream>
#include <switch.h>
#include <head.h>

void switchBranch(const std::string& branch_name){
//TODO: implement logic of updating working dir based on branch 
  std::string repo_root = arkDir();
  std::string branch_path = repo_root + "/.ark/refs/heads/" + branch_name;
  if(!(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path))){
    std::cerr<<"branch" << branch_name <<" does not exist\n";
    return;
  }
  std::string source_commit_hash = getHead();
  std::string target_commit_hash = getBranchHash(branch_name);
Commit* source_commit = new Commit();
source_commit->loadFromDisk(source_commit_hash);
Commit* target_commit = new Commit();
target_commit->loadFromDisk(target_commit_hash);
source_commit->tree->deleteFromWorkingDirectory(source_commit->tree->root,repo_root+"/");
target_commit->tree->writeToWorkingDirectory(target_commit->tree->root,repo_root+"/");
  std::cout<<"source :"<<source_commit_hash<<"\n";
  std::cout<<"target :"<<target_commit_hash<<"\n";
  std::cout<<"switched branch to "<<branch_name<<"\n";
}
