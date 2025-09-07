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

//use when wanna change head to point to a file in refs/heads
void updateHead(const std::string& branch_name){
  std::string repo_root = arkDir();
  std::string head_file = repo_root + "/.ark/HEAD";
  if(!(std::filesystem::exists(head_file) && std::filesystem::is_regular_file(head_file))){
    std::cerr<<"HEAD file not found\n";
  }
  std::ofstream out(head_file);
  if(!out){
    std::cerr<<"error reading branch file "<<head_file<<"\n";
  }
  out << "ref: refs/heads/"+branch_name;
  out.close();
}

//use when wanna make whatever brannch head is pointing to to point to a commit
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

std::string getBranchHash(const std::string& branch_name){
  std::string repo_root = arkDir();
  std::string branch_path = repo_root + "/.ark/refs/heads/" + branch_name;
  if(!(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path))){
    std::cerr<<"can't get branch hash as there is no branch "<<branch_name<<"\n";
  }
  std::ifstream in(branch_path);
  if(!in){
    std::cerr<<"error reading branch file "<<branch_path<<"\n";
  }
  std::string hash;
  in >> hash;
  in.close();
  return hash;
}
