#include <ark/objects/blob.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace ark {

Blob Blob::readFromFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cout << "Error: cannot open file " << path.string() << std::endl;
    return Blob{};
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return Blob{buffer.str()};
}

} // namespace ark
