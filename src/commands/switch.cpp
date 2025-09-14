#include <iostream>
#include <ark.h>
#include <filesystem>
#include <fstream>
#include <switch.h>
#include <head.h>
#include <objects.h>

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
  if(source_commit_hash == target_commit_hash){
    std::cerr<<"already on branch "<<branch_name<<std::endl;
    return;
  }
Commit* source_commit = new Commit();
source_commit->loadFromDisk(source_commit_hash);
Commit* target_commit = new Commit();
target_commit->loadFromDisk(target_commit_hash);
std::unordered_map<std::string,std::vector<std::pair<Object*,std::string>>> diff;
treeDiff(source_commit->tree->root,target_commit->tree->root,diff,"");
buildWorkingDirectoryFromTreeDiff(diff);
// source_commit->tree->deleteFromWorkingDirectory(source_commit->tree->root,repo_root+"/");
// target_commit->tree->writeToWorkingDirectory(target_commit->tree->root,repo_root+"/");
  std::cout<<"switched branch to "<<branch_name<<"\n";
  updateHead(branch_name);
  std::unordered_map<std::string,std::pair<std::string,std::string>> entries = target_commit->tree->flatten();
  writeToIndex(entries);
}
