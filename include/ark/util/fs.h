#ifndef ARK_UTIL_FS_H
#define ARK_UTIL_FS_H

#include <filesystem>
#include <string>
#include <string_view>

namespace ark {

// Returns an empty string when the file cannot be read.
std::string readFileToString(const std::filesystem::path &path);
std::string normalizeGenericPath(std::string_view path);

} // namespace ark

#endif
