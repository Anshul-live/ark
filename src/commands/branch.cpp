#include <branch.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark.h>
#include <head.h>

void branch(const std::string& name){
  std::string commit_hash = getHead();
  if(commit_hash == NULL_HASH){
    std::cerr<<"please make a commit first";
    return;
  }
  std::string refs_path = arkDir() + "/.ark/refs/" ;
  std::string branch_path = refs_path +"heads/" + name;
  if(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path)){
    std::cout<<"branch already exists"<<std::endl;
    return;
  }
  std::ofstream out(branch_path,std::ios::binary);
  if(!out){
    std::cerr<<"error creating branch";
    return;
  }
  std::string branch_base = getHead();
  out << branch_base;
  out.close();
}
