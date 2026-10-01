#ifndef ARK_OBJECTS_FLAT_TREE_H
#define ARK_OBJECTS_FLAT_TREE_H

#include <ark/core/repo_path.h>
#include <ark/objects/file_entry.h>

#include <map>

namespace ark {

using FlatTree = std::map<RepoPath, FileEntry>;

} // namespace ark

#endif
