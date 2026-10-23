#ifndef ARK_OBJECTS_FILE_ENTRY_H
#define ARK_OBJECTS_FILE_ENTRY_H

#include <ark/core/file_mode.h>
#include <ark/core/object_hash.h>

namespace ark {

struct FileEntry {
  ObjectHash hash;
  FileMode mode;

  friend bool operator==(const FileEntry &a, const FileEntry &b) {
    return a.hash == b.hash && a.mode == b.mode;
  }
  friend bool operator!=(const FileEntry &a, const FileEntry &b) {
    return !(a == b);
  }
};

} // namespace ark

#endif
