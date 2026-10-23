#ifndef ARK_OBJECTS_FLAT_TREE_H
#define ARK_OBJECTS_FLAT_TREE_H

#include <ark/core/repo_path.h>
#include <ark/objects/file_entry.h>

#include <ark/core/object_hash.h>

#include <map>

namespace ark {

class ObjectStore;

using FlatTree = std::map<RepoPath, FileEntry>;

// Every blob under the tree, keyed by its full path from the root.
FlatTree flattenTree(const ObjectStore &store, const ObjectHash &root);

} // namespace ark

#endif
