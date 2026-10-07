#include <ark/objects/blob.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace ark {

std::optional<Blob> Blob::tryReadFromFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return std::nullopt;
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return Blob{buffer.str()};
}

Blob Blob::readFromFile(const std::filesystem::path &path) {
  if (auto blob = tryReadFromFile(path))
    return *blob;
  std::cout << "Error: cannot open file " << path.string() << std::endl;
  return Blob{};
}

} // namespace ark
