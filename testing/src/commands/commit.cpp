#include <iostream>
#include <objects.h>
#include <filesystem>
#include <ark.h>
#include <head.h>

void commit(){
  std::string message = getCommitMessageFromEditor();
  std::string parent_commit_hash = getHead();
  Commit *commit = new Commit(message,parent_commit_hash);
  commit->writeObjectToDisk();
  setHead(commit->hash);
}
