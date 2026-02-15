#include <write-tree.h>
#include <objects.h>

int cmd_writeTree(const std::vector<std::string> &args){
  Tree* t = Tree::write();
  if(t){
    std::cout << t->root->hash << "\n";
  }
  return 0;
}
