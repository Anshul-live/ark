#include <status.h>
#include <objects.h>
#include <head.h>

bool status(){
  //whether there are any unstaged changes or not
  bool staged = true;
  std::unordered_map<std::string,Blob*> working_directory  = loadWorkingDirectoryWithoutIgnored();
  std::unordered_map<std::string,std::pair<std::string,std::string>> index = loadIndex();
  std::string commit_hash = getHead();
  Commit* commit = new Commit();
  commit->loadFromDisk(commit_hash);
  std::unordered_map<std::string,std::string> latest_commited_files = commit->tree->flatten();

  std::unordered_set<std::string> tracked;
  std::cout<<"Changes to be Commited:\n";
  for(const auto& [name,obj] : index){
    tracked.insert(obj.first);
    std::cout<<"\033[32m"<<name<<"\n";
  }
  std::cout<<"\n \033[0mChanges that will not be commited:\n";

  for(const auto& [name,obj]:working_directory){
    // std::cout<<index[name].first<<"\n";
    if(obj->hash == latest_commited_files[name] || tracked.find(obj->hash) == tracked.end()){
    std::cout<<"\033[31m"<<name<<"\n";
    staged = false;
    }
  }
  std::cout<<"\033[0m";
  return staged;
}
