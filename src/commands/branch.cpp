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

void printBranches(){
  std::string repo_root = arkDir();
  std::string refs_path = repo_root+ "/.ark/refs/";
  std::ifstream in(repo_root+"/.ark/HEAD");
  if(!in){
    std::cerr<<"unable to read HEAD";
    return;
  }
  std::string line;
  getline(in,line);
  in.close();
  std::string current_branch;
  if(line.rfind("ref: ",0) == 0){
    std::string branch_path = line.substr(5);
    current_branch = std::filesystem::relative(repo_root+"/.ark/"+branch_path,refs_path+"/heads/");
  }
  else{
    std::cout<<"HEAD is detached\n";
  }
  for (const auto& entry : std::filesystem::directory_iterator(refs_path+"heads/")) {
    if(std::filesystem::is_regular_file(entry)){
      std::string branch_name = std::filesystem::relative(entry,refs_path+"heads/").string();
      if(branch_name == current_branch){
        std::cout<<"* \033[32m"<<branch_name<<"\033[0m\n";
      }
      else{
        std::cout<<"  "<<branch_name<<"\n";
      }
    }
  }
}
