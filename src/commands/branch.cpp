#include <branch.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark.h>
#include <head.h>

int cmd_branch(const std::vector<std::string> &args){
  if(args.size() < 1){
      printBranches();
      return 0;
  }
  std::string name = args[0];
  std::string commit_hash = getHead();
  if(commit_hash == NULL_HASH){
    std::cerr<<"please make a commit first";
    return 1;
  }
  std::string refs_path = arkDir() + "/.ark/refs/" ;
  std::string branch_path = refs_path +"heads/" + name;
  if(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path)){
    std::cout<<"branch already exists"<<std::endl;
    return 1;
  }
  std::ofstream out(branch_path,std::ios::binary);
  if(!out){
    std::cerr<<"error creating branch";
    return 1;
  }
  std::string branch_base = getHead();
  out << branch_base;
  out.close();
  return 0;
}
