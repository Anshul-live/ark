#include <iostream>
#include <fstream>
#include <string>
#include <ark.h>
#include <head.h>

std::string getHead(){
  std::string repo_root = arkDir();
  std::string head_path = repo_root + "/.ark/HEAD";
  std::ifstream in(head_path);
  if(!in){
    std::cerr<<"error finding HEAD\n";
    return "";
  }
  std::string line;
  getline(in,line);
  if(line.rfind("ref: ",0) == 0){
    std::string branch_path = line.substr(5);
    std::ifstream branch_file(repo_root + "/.ark/" + branch_path);
    if(!branch_file){
      std::cerr<<"error opening branch file";
      return "";
    }
    std::string commit_hash;
    branch_file >> commit_hash;
    branch_file.close();
    return commit_hash;
  }
  in.close();
  return line;
}

void setHead(const std::string& commit_hash){
  std::string repo_root = arkDir();
  std::string head_path = repo_root + "/.ark/HEAD";
  std::ifstream in(head_path);
  if(!in){
    std::cerr<<"error finding HEAD\n";
    return;
  }
  std::string line;
  getline(in,line);
  in.close();
  if(line.rfind("ref: ",0) == 0){
    std::string branch_path = line.substr(5);
    std::ofstream branch_file(repo_root + "/.ark/" + branch_path,std::ios::binary);
    if(!branch_file){
      std::cerr<<"error opening branch file";
      return;
    }
    branch_file << commit_hash;
    branch_file.close();
    return;
  }

  std::ofstream out(head_path,std::ios::binary);
  if(!out){
    std::cerr<<"error updating HEAD\n";
    return;
  }
  out << commit_hash;
  out.close();
}
