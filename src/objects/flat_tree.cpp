#include <ark/objects/flat_tree.h>

#include <ark/objects/tree.h>
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>
#include <ark/util/profiler.h>

namespace ark {

namespace {

void flattenInto(const ObjectStore &store, const ObjectHash &hash,
                 const RepoPath &prefix, FlatTree &out) {
  ARK_PROF_SCOPE("tree.flatten.dir");
  std::string framed = store.readFramed(hash).value_or("");
  Tree tree = Tree::parse(parseObjectFrame(hash, framed).body);
  for (const TreeEntry &entry : tree.entries) {
    RepoPath path = prefix.child(entry.name);
    if (entry.type == ObjectType::Tree)
      flattenInto(store, entry.hash, path, out);
    else if (entry.type == ObjectType::Blob)
      out.insert_or_assign(path, FileEntry{entry.hash, entry.mode});
  }
}

} // namespace

FlatTree flattenTree(const ObjectStore &store, const ObjectHash &root) {
  FlatTree out;
  flattenInto(store, root, RepoPath(), out);
  return out;
}

} // namespace ark
