#ifndef ARK_STORAGE_OBJECT_FORMAT_H
#define ARK_STORAGE_OBJECT_FORMAT_H

#include <ark/core/object_hash.h>
#include <ark/core/object_type.h>
#include <string>
#include <string_view>

namespace ark {

struct RawObject {
  ObjectType type;
  std::string body;
};

// "<type> <size>\0<body>"
std::string frameObject(ObjectType type, std::string_view body);

// Throws ObjectFormatException when the frame is malformed.
RawObject parseObjectFrame(const ObjectHash &hash, std::string_view data);

} // namespace ark

#endif
