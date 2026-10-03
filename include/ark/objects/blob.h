#ifndef ARK_OBJECTS_BLOB_H
#define ARK_OBJECTS_BLOB_H

#include <filesystem>
#include <string>

namespace ark {

struct Blob {
  std::string data;

  // Reports an unreadable file on stdout and yields an empty blob.
  static Blob readFromFile(const std::filesystem::path &path);
};

} // namespace ark

#endif
