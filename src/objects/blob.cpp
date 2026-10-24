#include <ark/objects/blob.h>

#include <ark/util/profiler.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace ark {

std::optional<Blob> Blob::tryReadFromFile(const std::filesystem::path &path) {
  ARK_PROF_SCOPE("fs.read_file");
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return std::nullopt;
  std::ostringstream buffer;
  buffer << file.rdbuf();
  ARK_PROF_COUNT("files.read", 1);
  ARK_PROF_COUNT("bytes.read", buffer.str().size());
  return Blob{buffer.str()};
}

Blob Blob::readFromFile(const std::filesystem::path &path) {
  if (auto blob = tryReadFromFile(path))
    return *blob;
  std::cout << "Error: cannot open file " << path.string() << std::endl;
  return Blob{};
}

} // namespace ark
