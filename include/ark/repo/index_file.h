#ifndef ARK_REPO_INDEX_FILE_H
#define ARK_REPO_INDEX_FILE_H

#include <ark/objects/flat_tree.h>

#include <istream>
#include <ostream>

namespace ark {

// The on-disk text format of the index: one "mode hash path" line per entry.
class IndexFile {
public:
  static FlatTree parse(std::istream &in);
  static void write(std::ostream &out, const FlatTree &entries);
};

} // namespace ark

#endif
