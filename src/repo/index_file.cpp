#include <ark/core/exceptions.h>
#include <ark/repo/index_file.h>

#include <ark/core/file_mode.h>
#include <ark/util/profiler.h>

#include <string>
#include <string_view>

namespace ark {

FlatTree IndexFile::parse(std::istream &in) {
  ARK_PROF_SCOPE("index.parse");
  FlatTree entries;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    // Each line is "<mode> <hash> <path>". The path is everything after the
    // second space, so it may contain spaces itself.
    auto firstSpace = line.find(' ');
    auto secondSpace =
        firstSpace == std::string::npos ? firstSpace : line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
      throw IndexCorruptedException();
    auto parsedMode = parseFileMode(std::string_view(line).substr(0, firstSpace));
    if (!parsedMode)
      throw IndexCorruptedException();
    std::string hash = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string path = line.substr(secondSpace + 1);
    FileEntry entry{ObjectHash::unchecked(hash), *parsedMode};
    entries.insert_or_assign(RepoPath::fromRelative(path), entry);
  }
  return entries;
}

void IndexFile::write(std::ostream &out, const FlatTree &entries) {
  ARK_PROF_SCOPE("index.format");
  for (const auto &[path, entry] : entries) {
    out << toOctalString(entry.mode) << " " << entry.hash << " " << path
        << "\n";
  }
}

} // namespace ark
