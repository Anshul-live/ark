#include <ark/core/object_type.h>

namespace ark {

std::string_view toString(ObjectType type) {
  switch (type) {
  case ObjectType::Blob:
    return "blob";
  case ObjectType::Tree:
    return "tree";
  case ObjectType::Commit:
    return "commit";
  }
  return "";
}

std::optional<ObjectType> parseObjectType(std::string_view name) {
  if (name == "blob")
    return ObjectType::Blob;
  if (name == "tree")
    return ObjectType::Tree;
  if (name == "commit")
    return ObjectType::Commit;
  return std::nullopt;
}

} // namespace ark
