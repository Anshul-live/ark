#ifndef ARK_OBJECTS_BLOB_H
#define ARK_OBJECTS_BLOB_H

#include <filesystem>
#include <optional>
#include <string>

namespace ark {

struct Blob {
  std::string data;

  // Yields nullopt when the file cannot be opened. Prints nothing.
  static std::optional<Blob> tryReadFromFile(const std::filesystem::path &path);

  // Reports an unreadable file on stdout and yields an empty blob.
  static Blob readFromFile(const std::filesystem::path &path);
};

} // namespace ark

#endif
