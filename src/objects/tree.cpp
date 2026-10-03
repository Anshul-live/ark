#include <ark/objects/tree.h>

#include <ark/core/exceptions.h>

#include <algorithm>
#include <optional>

namespace ark {

std::string Tree::serialize() const {
  std::vector<const TreeEntry *> sorted;
  sorted.reserve(entries.size());
  for (const TreeEntry &entry : entries)
    sorted.push_back(&entry);
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const TreeEntry *a, const TreeEntry *b) {
                     return a->name < b->name;
                   });

  std::string out;
  for (const TreeEntry *entry : sorted) {
    out += toOctalString(entry->mode);
    out += ' ';
    out += toString(entry->type);
    out += ' ';
    out += entry->hash.hex();
    out += ' ';
    out += entry->name;
    out += '\n';
  }
  return out;
}

Tree Tree::parse(std::string_view body) {
  Tree tree;
  size_t pos = 0;
  while (pos < body.size()) {
    size_t eol = body.find('\n', pos);
    if (eol == std::string_view::npos)
      eol = body.size();
    std::string_view line = body.substr(pos, eol - pos);
    pos = eol + 1;

    size_t s1 = line.find(' ');
    size_t s2 = s1 == std::string_view::npos ? s1 : line.find(' ', s1 + 1);
    size_t s3 = s2 == std::string_view::npos ? s2 : line.find(' ', s2 + 1);
    if (s3 == std::string_view::npos)
      throw InvalidObjectException("malformed tree entry");

    std::optional<ObjectType> type =
        parseObjectType(line.substr(s1 + 1, s2 - s1 - 1));
    if (!type)
      throw InvalidObjectException("unknown type in tree entry");
    FileMode mode =
        parseFileMode(line.substr(0, s1)).value_or(FileMode::Regular);

    tree.entries.push_back(
        TreeEntry{std::string(line.substr(s3 + 1)), mode, *type,
                  ObjectHash::unchecked(
                      std::string(line.substr(s2 + 1, s3 - s2 - 1)))});
  }
  return tree;
}

} // namespace ark
