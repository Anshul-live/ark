#ifndef ARK_OBJECTS_COMMIT_STORE_H
#define ARK_OBJECTS_COMMIT_STORE_H

#include <ark/core/object_hash.h>
#include <ark/objects/commit.h>

namespace ark {

class ObjectStore;

class CommitStore {
public:
  explicit CommitStore(ObjectStore &store) : store_(store) {}

  // Throws ObjectFormatException when the object is missing or malformed.
  Commit read(const ObjectHash &hash) const;
  ObjectHash write(const Commit &commit);

private:
  ObjectStore &store_;
};

} // namespace ark

#endif
