#include <ark/objects/commit_store.h>
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>

namespace ark {

Commit CommitStore::read(const ObjectHash &hash) const {
  std::string framed = store_.readFramed(hash).value_or("");
  return Commit::parse(parseObjectFrame(hash, framed).body);
}

ObjectHash CommitStore::write(const Commit &commit) {
  return store_.write(ObjectType::Commit, commit.serialize());
}

} // namespace ark
