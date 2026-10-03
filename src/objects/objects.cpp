// represents a single file
#include <ark/core/exceptions.h>
#include <_stdio.h>
#include <ark/cli/commit_message_editor.h>
#include <ark/cli/commands.h>
#include <charconv>
#include <ark/storage/compression.h>
#include <ark/repo/config.h>
#include <ark/util/strings.h>
#include <ark/util/time.h>
#include <filesystem>
#include <fstream>
#include <ark/repo/index.h>
#include <iomanip>
#include <iostream>
#include <ark/objects/objects.h>
#include <ark/objects/tree_builder.h>
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>
#include <ark/storage/sha256.h>
#include <ark/repo/repository.h>
#include <sstream>
#include <string>
#include <system_error>

Commit::Commit() {}

Commit::Commit(const std::string &message, const std::string &parent_hash) {
  Repository repo;
  Config config;
  config.load();
  Index idx;
  idx.load();
  ark::ObjectStore store(repo.objectsDir());
  this->treeHash = ark::writeTreeFromIndex(idx, store).hex();
  if (!config.hasUserConfig()) {
    std::cerr << "please provide the following values user.name and user.email "
                 "before committing.\n";
    exit(0);
  }
  std::string name = config.getUserName();
  std::string email = config.getUserEmail();
  std::cout << "passed checks" << std::endl;
  long long timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::string timezone_offset = ark::formatLocalUtcOffset();

  std::ostringstream buffer;
  buffer << "tree " << this->treeHash << "\n";
  buffer << "parent " << parent_hash << "\n";
  buffer << "committer " << name << " <" << email << "> " << timestamp << " "
         << timezone_offset << "\n";

  buffer << message << "\n";
  std::string raw_content = buffer.str();
  this->content = ark::frameObject(ark::ObjectType::Commit, raw_content);
  this->hash = this->getSha256();
}

Commit::Commit(const std::string &message, const std::string &parent1_hash,
               const std::string &parent2_hash) {
  Repository repo;
  Config config;
  config.load();
  Index idx;
  idx.load();
  ark::ObjectStore store(repo.objectsDir());
  this->treeHash = ark::writeTreeFromIndex(idx, store).hex();
  if (!config.hasUserConfig()) {
    std::cerr << "please provide the following values user.name and user.email "
                 "before committing.\n";
    exit(0);
  }
  std::string name = config.getUserName();
  std::string email = config.getUserEmail();
  long long timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::string timezone_offset = ark::formatLocalUtcOffset();

  std::ostringstream buffer;
  buffer << "tree " << this->treeHash << "\n";
  buffer << "parent " << parent1_hash << "\n";
  buffer << "parent " << parent2_hash << "\n";
  buffer << "committer " << name << " <" << email << "> " << timestamp << " "
         << timezone_offset << "\n";

  buffer << message << "\n";
  std::string raw_content = buffer.str();
  this->content = ark::frameObject(ark::ObjectType::Commit, raw_content);
  this->hash = this->getSha256();
}

void Commit::loadFromDisk(const std::string &hash) {
  if (hash == NULL_HASH)
    return;
  std::string raw = Object::readFromDisk(hash);
  std::string content = Object::stripHeaderFromObject(hash, raw);
  std::stringstream content_stream(content);
  std::string line;
  getline(content_stream, line);
  Config config;
  std::vector<std::string> tree_info = ark::text::splitKeepEmpty(line, ' ');

  parents.clear();
  line.clear();
  getline(content_stream, line);
  while (ark::text::startsWith(line, "parent")) {
    std::vector<std::string> values = ark::text::splitKeepEmpty(line, ' ');
    parents.push_back(values[1]);
    line.clear();
    getline(content_stream, line);
  }
  this->treeHash = tree_info[1];
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  this->files = ark::flattenTree(store, ark::ObjectHash::unchecked(tree_info[1]));
}

Commit *Commit::create(const std::string &treeHash,
                       const std::string &parent1Hash,
                       const std::string &parent2Hash,
                       const std::string &message) {
  Commit *commit = new Commit();

  commit->treeHash = treeHash;
  {
    Repository treeRepo;
    ark::ObjectStore treeStore(treeRepo.objectsDir());
    commit->files =
        ark::flattenTree(treeStore, ark::ObjectHash::unchecked(treeHash));
  }

  Config config;
  config.load();
  if (!config.hasUserConfig()) {
    std::cerr << "please provide user.name and user.email before committing.\n";
    exit(0);
  }

  std::string name = config.getUserName();
  std::string email = config.getUserEmail();
  long long timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  Repository repo;
  std::string timezone_offset = ark::formatLocalUtcOffset();

  std::ostringstream buffer;
  buffer << "tree " << treeHash << "\n";
  if (!parent1Hash.empty()) {
    buffer << "parent " << parent1Hash << "\n";
  }
  if (!parent2Hash.empty()) {
    buffer << "parent " << parent2Hash << "\n";
  }
  buffer << "committer " << name << " <" << email << "> " << timestamp << " "
         << timezone_offset << "\n";
  buffer << message << "\n";

  std::string raw_content = buffer.str();
  commit->content = ark::frameObject(ark::ObjectType::Commit, raw_content);
  commit->hash = commit->getSha256();
  commit->writeObjectToDisk();

  return commit;
}

std::string Object::readFromDisk(const std::string &hash) {
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  return store.readFramed(ark::ObjectHash::unchecked(hash)).value_or("");
}

std::string Object::stripHeaderFromObject(const std::string &hash,
                                          const std::string &data) {
  return ark::parseObjectFrame(ark::ObjectHash::unchecked(hash), data).body;
}

bool Object::isWrittenToDisk() {
  Repository repo;
  return ark::ObjectStore(repo.objectsDir())
      .contains(ark::ObjectHash::unchecked(hash));
}

std::string Object::getSha256() { return ark::sha256Hex(content); }

void Object::writeObjectToDisk() {
  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  ark::RawObject raw =
      ark::parseObjectFrame(ark::ObjectHash::unchecked(hash), content);
  store.write(raw.type, raw.body);
}
