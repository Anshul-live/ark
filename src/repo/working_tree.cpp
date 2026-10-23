#include <ark/repo/working_tree.h>

#include <ark/core/file_mode.h>
#include <ark/objects/blob.h>
#include <ark/storage/object_store.h>

namespace ark {

FlatTree WorkingTree::scan() const {
  FlatTree files;
  const std::filesystem::path root = repo_.root();
  forEachFile(root, [&](const std::filesystem::path &path) {
    Blob blob = Blob::readFromFile(path);
    FileMode mode = detectFileMode(path).value_or(FileMode::Regular);
    files.insert_or_assign(
        RepoPath::fromRelative(std::filesystem::relative(path, root).string()),
        FileEntry{ObjectStore::hashOf(ObjectType::Blob, blob.data), mode});
    return true;
  });
  return files;
}

} // namespace ark
