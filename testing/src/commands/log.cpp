#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <cat-file.h>
#include <ark.h>
#include <log.h>
#include <objects.h>

void logBranch(std::string branch_name,int depth){
  std::string repo_root = arkDir();
  std::string refs_path = repo_root+"/.ark/refs/heads/";
  if(!(std::filesystem::exists(refs_path+branch_name) && std::filesystem::is_regular_file(refs_path+branch_name))){
    std::cerr<<"branch does not exist";
    return;
  }
  std::ifstream in(refs_path+branch_name);
  if(!in){
    std::cerr<<"error opening file"<<refs_path+branch_name;
    return;
  }
  std::string commit_hash;
  in >> commit_hash;
  in.close();
  while(commit_hash != NULL_HASH &&(depth == -1 || depth--)){
    std::string content = catFile(commit_hash);
    std::vector<std::string> lines = split(content,'\n');
    std::vector<std::string> data;
    std::cout<<"\033[1;33mcommit: "<<commit_hash<<"\033[0m\n";
    for(auto line:lines){
      std::vector<std::string> temp = split(line,' ');
      data.insert(data.end(),temp.begin(),temp.end());
    }
    commit_hash = data[3];
    //TODO: fix the parsing logic its not right for message value cause space splitting splits the message
    //and you only get the last word of commit message
    // std::cout<<"\n"<<data[data.size() - 1]<<"\n\n";
  }
}

void log(){
  std::string repo_root = arkDir();
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
    current_branch = std::filesystem::relative(repo_root+"/.ark/"+branch_path,repo_root+"/.ark/refs/heads/");
  }
  else{
    std::cout<<"HEAD is detached\n";
    return;
  }
  logBranch(current_branch,-1);
}
