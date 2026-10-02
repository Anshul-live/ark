#ifndef ARK_STORAGE_OBJECT_STORE_H
#define ARK_STORAGE_OBJECT_STORE_H

#include <ark/core/object_hash.h>
#include <ark/core/object_type.h>
#include <ark/storage/object_format.h>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ark {

// Loose objects at <objectsDir>/<2 hex>/<62 hex>, zlib-compressed.
class ObjectStore {
public:
  explicit ObjectStore(std::filesystem::path objectsDir);

  bool contains(const ObjectHash &hash) const;
  // Nullopt when the object file is missing.
  std::optional<RawObject> read(const ObjectHash &hash) const;
  // The decompressed framed bytes; nullopt when the object file is missing.
  std::optional<std::string> readFramed(const ObjectHash &hash) const;
  ObjectHash write(ObjectType type, std::string_view body);
  static ObjectHash hashOf(ObjectType type, std::string_view body);

private:
  std::filesystem::path pathFor(const ObjectHash &hash) const;
  std::filesystem::path objectsDir_;
};

} // namespace ark

#endif
