#include <iostream>
#include <objects.h>
#include <filesystem>
#include <ark.h>
#include <head.h>

int cmd_commit(const std::vector<std::string> &args){
  std::string commit_hash = getHead();
  if(isIndexSameAsCommit(commit_hash)){
    std::cout<<"no changes to commit";
    return 0;
  }
  std::string message = getCommitMessageFromEditor();
  if(message.empty()){
    std::cerr<<"commit aborted due to empty message";
    return 1;
  }
  std::string parent_commit_hash = getHead();
  Commit *commit = new Commit(message,parent_commit_hash);
  commit->writeObjectToDisk();
  setHead(commit->hash);
  return 0;
}
