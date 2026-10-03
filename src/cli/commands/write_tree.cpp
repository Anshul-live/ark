#include <iostream>
#include <ark/cli/commands.h>
#include <ark/objects/tree_builder.h>
#include <ark/repo/index.h>
#include <ark/repo/repository.h>
#include <ark/storage/object_store.h>

int cmd_writeTree(const std::vector<std::string> &args){
  Repository repo;
  Index idx;
  idx.load();
  ark::ObjectStore store(repo.objectsDir());
  std::cout << ark::writeTreeFromIndex(idx, store) << "\n";
  return 0;
}
