#include <ark/storage/object_store.h>

#include <ark/storage/compression.h>
#include <ark/storage/sha256.h>
#include <ark/util/profiler.h>
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
  ARK_PROF_SCOPE("store.read");
  ARK_PROF_COUNT("objects.read", 1);
  std::filesystem::path file = pathFor(hash);
  if (!std::filesystem::exists(file))
    return std::nullopt;

  std::string compressed;
  {
    ARK_PROF_SCOPE("store.read.io");
    std::ifstream in(file, std::ios::binary);
    if (!in)
      return std::nullopt;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    compressed = buffer.str();
  }
  ARK_PROF_COUNT("bytes.read_compressed", compressed.size());
  ARK_PROF_SCOPE("store.read.inflate");
  return decompressObject(compressed);
}

std::optional<RawObject> ObjectStore::read(const ObjectHash &hash) const {
  std::optional<std::string> framed = readFramed(hash);
  if (!framed)
    return std::nullopt;
  return parseObjectFrame(hash, *framed);
}

ObjectHash ObjectStore::hashOf(ObjectType type, std::string_view body) {
  ARK_PROF_SCOPE("store.hash");
  return ObjectHash::fromDigest(sha256Digest(frameObject(type, body)));
}

ObjectHash ObjectStore::write(ObjectType type, std::string_view body) {
  ARK_PROF_SCOPE("store.write");
  ARK_PROF_COUNT("objects.written", 1);
  std::string framed = frameObject(type, body);
  ObjectHash hash = [&framed] {
    ARK_PROF_SCOPE("store.write.hash");
    return ObjectHash::fromDigest(sha256Digest(framed));
  }();
  bool exists;
  {
    ARK_PROF_SCOPE("store.write.stat");
    exists = contains(hash);
  }
  if (exists) {
    ARK_PROF_COUNT("objects.dedup_hits", 1);
    return hash;
  }

  std::filesystem::path file = pathFor(hash);
  {
    ARK_PROF_SCOPE("store.write.mkdir");
    std::filesystem::create_directories(file.parent_path());
  }
  std::string compressed;
  {
    ARK_PROF_SCOPE("store.write.deflate");
    compressed = compressObject(framed);
  }
  ARK_PROF_COUNT("bytes.framed", framed.size());
  ARK_PROF_COUNT("bytes.stored", compressed.size());
  ARK_PROF_SCOPE("store.write.io");
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
