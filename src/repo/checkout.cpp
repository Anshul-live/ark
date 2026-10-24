#include <ark/repo/checkout.h>

#include <ark/repo/repository.h>
#include <ark/util/profiler.h>
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace ark {

Checkout::Checkout(const Repository &repo, const ObjectStore &store)
    : root_(repo.root()), store_(store) {}

bool Checkout::createFile(const std::string &path, FileMode mode,
                const std::string &data) {
  std::cout << "creating file " << path << "\n";
  std::filesystem::path p(path);
  std::filesystem::create_directories(p.parent_path());

  if (mode == FileMode::Regular || mode == FileMode::Executable) {
    std::ofstream out(p);
    if (!out) {
      std::cerr << "Failed to create file: " << p << "\n";
      return false;
    }
    out << data;
    out.close();

    auto perms = (mode == FileMode::Regular)
                     ? (std::filesystem::perms::owner_read |
                        std::filesystem::perms::owner_write |
                        std::filesystem::perms::group_read |
                        std::filesystem::perms::others_read)
                     : (std::filesystem::perms::owner_all |
                        std::filesystem::perms::group_read |
                        std::filesystem::perms::group_exec |
                        std::filesystem::perms::others_read |
                        std::filesystem::perms::others_exec);

    std::filesystem::permissions(p, perms,
                                 std::filesystem::perm_options::replace);
    return true;
  }

  if (mode == FileMode::Symlink) {
    try {
      std::filesystem::create_symlink(data, path);
    } catch (const std::exception &e) {
      std::cerr << "Failed to create symlink: " << e.what() << "\n";
      return false;
    }
    return true;
  }
  return false;
}

bool Checkout::removeFile(const std::string &path) {
  std::cout << "deleting file " << path << "\n";
  std::filesystem::path p(path);
  if (std::filesystem::exists(p)) {
    bool deleted = std::filesystem::remove(p);
    if (!deleted) {
      std::cerr << "unable to delete file " << p.string() << "\n";
      return false;
    }
    return true;
  }
  return false;
}

bool Checkout::overwriteFile(const std::string &path, FileMode mode,
                           const std::string &data) {
  std::cout << "overwriting file " << path << "\n";
  std::filesystem::path p(path);
  if (!std::filesystem::exists(p)) {
    std::cout << "file not found for overwriting falling back to creation\n";
    createFile(path, mode, data);
    return false;
  }
  std::ofstream out(p);
  if (!out) {
    std::cerr << "error opening file " << p.string() << std::endl;
    return false;
  }
  out << data;
  out.close();
  return true;
}

std::string Checkout::fullPath(const RepoPath &path) const {
  return root_ + "/" + path.str();
}

std::string Checkout::blobData(const ObjectHash &hash) const {
  ARK_PROF_SCOPE("checkout.blob");
  std::string framed = store_.readFramed(hash).value_or("");
  return parseObjectFrame(hash, framed).body;
}

void Checkout::apply(const std::vector<FileChange> &changes) {
  ARK_PROF_SCOPE("checkout.apply");
  ARK_PROF_COUNT("checkout.changes", changes.size());
  for (const FileChange &change : changes)
    if (change.kind == ChangeKind::Deleted)
      removeFile(fullPath(change.path));
  for (const FileChange &change : changes)
    if (change.kind == ChangeKind::Added)
      createFile(fullPath(change.path), change.after->mode,
                 blobData(change.after->hash));
  for (const FileChange &change : changes)
    if (change.kind == ChangeKind::Modified)
      overwriteFile(fullPath(change.path), change.after->mode,
                    blobData(change.after->hash));
}

} // namespace ark
