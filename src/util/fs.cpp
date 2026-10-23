#include <ark/util/fs.h>
#include <fstream>
#include <sstream>

namespace ark {

std::string readFileToString(const std::filesystem::path &path) {
  std::ifstream in(path);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string normalizeGenericPath(std::string_view path) {
  return std::filesystem::path(path).lexically_normal().generic_string();
}

} // namespace ark
