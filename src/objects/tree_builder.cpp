#include <ark/objects/tree_builder.h>

#include <ark/objects/tree.h>
#include <ark/repo/index.h>
#include <ark/storage/object_store.h>
#include <ark/util/profiler.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ark {

struct TreeBuilder::Node {
  std::map<std::string, std::unique_ptr<Node>> children;
  std::optional<FileEntry> leaf;
};

TreeBuilder::TreeBuilder() : root_(std::make_unique<Node>()) {}
TreeBuilder::~TreeBuilder() = default;
TreeBuilder::TreeBuilder(TreeBuilder &&) noexcept = default;
TreeBuilder &TreeBuilder::operator=(TreeBuilder &&) noexcept = default;

void TreeBuilder::add(const RepoPath &path, FileEntry entry) {
  std::vector<std::string> parts = path.components();
  if (parts.empty())
    return;

  Node *node = root_.get();
  for (size_t i = 0; i + 1 < parts.size(); i++) {
    std::unique_ptr<Node> &child = node->children[parts[i]];
    if (!child)
      child = std::make_unique<Node>();
    child->leaf.reset();
    node = child.get();
  }
  std::unique_ptr<Node> &last = node->children[parts.back()];
  last = std::make_unique<Node>();
  last->leaf = entry;
}

TreeBuilder TreeBuilder::fromFlatTree(const FlatTree &files) {
  TreeBuilder builder;
  for (const auto &[path, entry] : files)
    builder.add(path, entry);
  return builder;
}

ObjectHash TreeBuilder::writeNode(const Node &node, ObjectStore &store) {
  ARK_PROF_SCOPE("tree.write.dir");
  Tree tree;
  for (const auto &[name, child] : node.children) {
    if (child->leaf) {
      tree.entries.push_back(
          TreeEntry{name, child->leaf->mode, ObjectType::Blob,
                    child->leaf->hash});
    } else {
      tree.entries.push_back(TreeEntry{name, FileMode::Directory,
                                       ObjectType::Tree,
                                       writeNode(*child, store)});
    }
  }
  return store.write(ObjectType::Tree, tree.serialize());
}

ObjectHash TreeBuilder::write(ObjectStore &store) const {
  return writeNode(*root_, store);
}

ObjectHash writeTreeFromIndex(const ::Index &index, ObjectStore &store) {
  ARK_PROF_SCOPE("tree.build");
  TreeBuilder builder;
  {
    ARK_PROF_SCOPE("tree.build.insert");
    builder = TreeBuilder::fromFlatTree(index.entries());
  }
  return builder.write(store);
}

} // namespace ark
