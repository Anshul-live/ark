#ifndef ARK_OBJECTS_TREE_BUILDER_H
#define ARK_OBJECTS_TREE_BUILDER_H

#include <ark/core/object_hash.h>
#include <ark/core/repo_path.h>
#include <ark/objects/file_entry.h>
#include <ark/objects/flat_tree.h>

#include <memory>

class Index;

namespace ark {

class ObjectStore;

class TreeBuilder {
public:
  TreeBuilder();
  ~TreeBuilder();
  TreeBuilder(TreeBuilder &&) noexcept;
  TreeBuilder &operator=(TreeBuilder &&) noexcept;

  void add(const RepoPath &path, FileEntry entry);
  // Writes every subtree bottom-up and returns the root tree's hash.
  ObjectHash write(ObjectStore &store) const;
  static TreeBuilder fromFlatTree(const FlatTree &files);

private:
  struct Node;
  static ObjectHash writeNode(const Node &node, ObjectStore &store);

  std::unique_ptr<Node> root_;
};

ObjectHash writeTreeFromIndex(const ::Index &index, ObjectStore &store);

} // namespace ark

#endif
