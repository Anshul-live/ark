#include <iostream>
#include <objects.h>
#include <filesystem>
#include <ark.h>
#include <head.h>

void commit(){
  std::string commit_hash = getHead();
  if(isIndexSameAsCommit(commit_hash)){
    std::cout<<"no changes to commit";
    return;
  }
  std::string message = getCommitMessageFromEditor();
  if(message.empty()){
    std::cerr<<"commit aborted due to empty message";
    return;
  }
  std::string parent_commit_hash = getHead();
  Commit *commit = new Commit(message,parent_commit_hash);
  commit->writeObjectToDisk();
  setHead(commit->hash);
}
