#ifndef ARK_OBJECTS_CHECKOUT_LEGACY_H
#define ARK_OBJECTS_CHECKOUT_LEGACY_H

#include <ark/objects/file_entry.h>
#include <ark/objects/flat_tree.h>

#include <string>
#include <vector>

namespace ark {

class ObjectStore;

struct CheckoutChange {
  std::string path;
  FileEntry entry;
};

struct CheckoutPlan {
  std::vector<CheckoutChange> remove;
  std::vector<CheckoutChange> create;
  std::vector<CheckoutChange> overwrite;
};

// Paths in the plan are "<root>/<repo path>".
CheckoutPlan planCheckout(const FlatTree &from, const FlatTree &to,
                          const std::string &root);
void applyCheckout(const CheckoutPlan &plan, const ObjectStore &store);

bool createFile(const std::string &path, FileMode mode,
                const std::string &data);
bool deleteFile(const std::string &path);
bool overwriteFile(const std::string &path, FileMode mode,
                   const std::string &data);

} // namespace ark

#endif
