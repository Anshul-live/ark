#ifndef OBJECTS_HPP
#define OBJECTS_HPP

#include <ark/cli/commit_message_editor.h>
#include <ark/objects/flat_tree.h>
#include <ark/storage/compression.h>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Object {
public:
  std::string mode;
  std::string hash;
  std::string content;
  std::string name;

  virtual ~Object() = default;
  Object() = default;

  std::string getSha256();
  void writeObjectToDisk();
  bool isWrittenToDisk();

  static std::string readFromDisk(const std::string &hash);
  static std::string stripHeaderFromObject(const std::string &hash,
                                           const std::string &data);
};

class Commit : public Object {
public:
  std::string treeHash;
  ark::FlatTree files;
  std::vector<std::string> parents;
  Commit();
  Commit(const std::string &message, const std::string &parent_hash);
  Commit(const std::string &message, const std::string &parent1_hash,
         const std::string &parent2_hash);
  void loadFromDisk(const std::string &hash);

  static Commit *create(const std::string &treeHash,
                        const std::string &parent1Hash,
                        const std::string &parent2Hash,
                        const std::string &message);
};

#endif
