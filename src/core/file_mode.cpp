#include <ark/core/file_mode.h>

namespace ark {

std::string_view toOctalString(FileMode mode) {
  switch (mode) {
  case FileMode::Regular:
    return "100644";
  case FileMode::Executable:
    return "100755";
  case FileMode::Symlink:
    return "120000";
  case FileMode::Directory:
    return "040000";
  }
  return "";
}

std::optional<FileMode> parseFileMode(std::string_view text) {
  if (text == "100644")
    return FileMode::Regular;
  if (text == "100755")
    return FileMode::Executable;
  if (text == "120000")
    return FileMode::Symlink;
  if (text == "040000")
    return FileMode::Directory;
  return std::nullopt;
}

std::optional<FileMode> detectFileMode(const std::filesystem::path &path) {
  std::error_code ec;
  auto st = std::filesystem::symlink_status(path, ec);
  if (ec)
    return std::nullopt;
  if (std::filesystem::is_symlink(st))
    return FileMode::Symlink;
  if (std::filesystem::is_directory(st))
    return FileMode::Directory;
  if (std::filesystem::is_regular_file(st)) {
    auto perms = st.permissions();
    if ((perms & std::filesystem::perms::owner_exec) !=
        std::filesystem::perms::none)
      return FileMode::Executable;
    return FileMode::Regular;
  }
  return std::nullopt;
}

} // namespace ark
