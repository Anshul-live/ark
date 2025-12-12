#include <write-tree.h>


int cmd_writeTree(const std::vector<std::string> &args){
  Tree* t = writeTree();
  if(t){
    std::cout << t->root->hash << "\n";
  }
  return 0;
}

Tree* writeTree(){
  Tree* t = new Tree();
  t->writeTreeToDisk(t->root);
  return t;
}
