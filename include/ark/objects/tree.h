#ifndef ARK_OBJECTS_TREE_H
#define ARK_OBJECTS_TREE_H

#include <ark/core/file_mode.h>
#include <ark/core/object_hash.h>
#include <ark/core/object_type.h>

#include <string>
#include <string_view>
#include <vector>

namespace ark {

struct TreeEntry {
  std::string name;
  FileMode mode;
  ObjectType type;
  ObjectHash hash;
};

class Tree {
public:
  std::vector<TreeEntry> entries;

  // One "<mode> <type> <hash> <name>\n" line per entry, sorted by name.
  std::string serialize() const;
  // Throws InvalidObjectException on a malformed entry line.
  static Tree parse(std::string_view body);
};

} // namespace ark

#endif
