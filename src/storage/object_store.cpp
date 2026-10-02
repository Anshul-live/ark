#include <ark/storage/object_store.h>

#include <ark/storage/compression.h>
#include <ark/storage/sha256.h>
#include <fstream>
#include <sstream>

namespace ark {

ObjectStore::ObjectStore(std::filesystem::path objectsDir)
    : objectsDir_(std::move(objectsDir)) {}

std::filesystem::path ObjectStore::pathFor(const ObjectHash &hash) const {
  return objectsDir_ / std::string(hash.fanoutDir()) /
         std::string(hash.fanoutFile());
}

bool ObjectStore::contains(const ObjectHash &hash) const {
  return std::filesystem::exists(pathFor(hash));
}

std::optional<std::string> ObjectStore::readFramed(const ObjectHash &hash) const {
  std::filesystem::path file = pathFor(hash);
  if (!std::filesystem::exists(file))
    return std::nullopt;

  std::ifstream in(file, std::ios::binary);
  if (!in)
    return std::nullopt;

  std::ostringstream buffer;
  buffer << in.rdbuf();
  return decompressObject(buffer.str());
}

std::optional<RawObject> ObjectStore::read(const ObjectHash &hash) const {
  std::optional<std::string> framed = readFramed(hash);
  if (!framed)
    return std::nullopt;
  return parseObjectFrame(hash, *framed);
}

ObjectHash ObjectStore::hashOf(ObjectType type, std::string_view body) {
  return ObjectHash::fromDigest(sha256Digest(frameObject(type, body)));
}

ObjectHash ObjectStore::write(ObjectType type, std::string_view body) {
  std::string framed = frameObject(type, body);
  ObjectHash hash = ObjectHash::fromDigest(sha256Digest(framed));
  if (contains(hash))
    return hash;

  std::filesystem::path file = pathFor(hash);
  std::filesystem::create_directories(file.parent_path());
  std::string compressed = compressObject(framed);
  std::ofstream out(file, std::ios::binary);
  if (!compressed.empty()) {
    out.write(compressed.data(), static_cast<std::streamsize>(compressed.size()));
  } else {
    out.write(framed.data(), static_cast<std::streamsize>(framed.size()));
  }
  out.close();
  return hash;
}

} // namespace ark
