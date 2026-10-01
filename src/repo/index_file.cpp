#include <ark/core/exceptions.h>
#include <ark/repo/index_file.h>

#include <string>

namespace ark {

FlatTree IndexFile::parse(std::istream &in) {
  FlatTree entries;
  std::string mode, hash, path;
  while (in >> mode >> hash >> path) {
    auto parsedMode = parseFileMode(mode);
    if (!parsedMode)
      throw IndexCorruptedException();
    FileEntry entry{ObjectHash::unchecked(hash), *parsedMode};
    entries.insert_or_assign(RepoPath::fromRelative(path), entry);
  }
  return entries;
}

void IndexFile::write(std::ostream &out, const FlatTree &entries) {
  for (const auto &[path, entry] : entries) {
    out << toOctalString(entry.mode) << " " << entry.hash << " " << path
        << "\n";
  }
}

} // namespace ark
