#include <write-tree.h>

Tree* writeTree(){
  Tree* t = new Tree();
  t->writeTreeToDisk(t->root);
  return t;
}
