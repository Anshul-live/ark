#include <ark/objects/checkout_legacy.h>

#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace ark {

bool createFile(const std::string &path, FileMode mode,
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

bool deleteFile(const std::string &path) {
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

bool overwriteFile(const std::string &path, FileMode mode,
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

CheckoutPlan planCheckout(const FlatTree &from, const FlatTree &to,
                          const std::string &root) {
  CheckoutPlan plan;
  auto fullPath = [&](const RepoPath &path) { return root + "/" + path.str(); };

  for (const auto &[path, entry] : from) {
    if (to.find(path) == to.end())
      plan.remove.push_back({fullPath(path), entry});
  }
  for (const auto &[path, entry] : to) {
    auto it = from.find(path);
    if (it == from.end())
      plan.create.push_back({fullPath(path), entry});
    else if (it->second.hash != entry.hash)
      plan.overwrite.push_back({fullPath(path), entry});
  }
  return plan;
}

namespace {

std::string readBlobData(const ObjectStore &store, const ObjectHash &hash) {
  std::string framed = store.readFramed(hash).value_or("");
  return parseObjectFrame(hash, framed).body;
}

} // namespace

void applyCheckout(const CheckoutPlan &plan, const ObjectStore &store) {
  for (const CheckoutChange &change : plan.remove)
    deleteFile(change.path);
  for (const CheckoutChange &change : plan.create)
    createFile(change.path, change.entry.mode,
               readBlobData(store, change.entry.hash));
  for (const CheckoutChange &change : plan.overwrite)
    overwriteFile(change.path, change.entry.mode,
                  readBlobData(store, change.entry.hash));
}

} // namespace ark
