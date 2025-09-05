#include <iostream>
#include <objects.h>
#include <filesystem>
#include <ark.h>

void commit(){
  std::string message = getCommitMessageFromEditor();
  Commit *commit = new Commit(message);
  commit->writeObjectToDisk();
  std::cout<<commit->hash<<"\n";
}
