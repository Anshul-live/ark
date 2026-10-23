#ifndef ARK_CORE_FILE_MODE_H
#define ARK_CORE_FILE_MODE_H

#include <filesystem>
#include <optional>
#include <string_view>

namespace ark {

enum class FileMode { Regular, Executable, Symlink, Directory };

std::string_view toOctalString(FileMode mode);
std::optional<FileMode> parseFileMode(std::string_view text);

// Mirrors Repository::getMode: a regular file is Executable when the owner
// execute bit is set. Returns nullopt for missing paths and special files.
std::optional<FileMode> detectFileMode(const std::filesystem::path &path);

} // namespace ark

#endif
