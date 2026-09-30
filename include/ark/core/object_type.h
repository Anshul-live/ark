#ifndef ARK_CORE_OBJECT_TYPE_H
#define ARK_CORE_OBJECT_TYPE_H

#include <optional>
#include <string_view>

namespace ark {

enum class ObjectType { Blob, Tree, Commit };

std::string_view toString(ObjectType type);
std::optional<ObjectType> parseObjectType(std::string_view name);

} // namespace ark

#endif
