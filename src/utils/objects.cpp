// represents a single file
#include <_stdio.h>
#include <ark.h>
#include <cat-file.h>
#include <compress.h>
#include <config.h>
#include <filesystem>
#include <fstream>
#include <hash-object.h>
#include <index.h>
#include <iomanip>
#include <iostream>
#include <objects.h>
#include <openssl/sha.h>
#include <repository.h>
#include <sstream>
#include <string>

Blob::Blob(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cout << "Error: cannot open file " << filename << std::endl;
    return;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  std::string file_content = buffer.str();

  // Create Git-like blob content (blob header + content)
  this->content = "blob " + std::to_string(file_content.size()) +
                  std::string("\0", 1) + file_content;

  this->hash = this->getSha256();
}

Blob *Blob::fromFile(const std::string &filename) { return new Blob(filename); }

void Blob::loadFromDisk(const std::string &hash) {
  std::string content = Object::readFromDisk(hash);
  this->content = content;
}

bool Blob::createFile(const std::string &path) {
  std::cout << "creating file " << path + this->name << "\n";
  std::filesystem::path p(path + this->name);
  std::filesystem::create_directories(p.parent_path());

  if (mode == "100644" || mode == "100755") {
    std::ofstream out(p);
    if (!out) {
      std::cerr << "Failed to create file: " << p << "\n";
      return false;
    }
    out << this->content;
    out.close();

    // Apply permissions
    auto perms = (mode == "100644") ? (std::filesystem::perms::owner_read |
                                       std::filesystem::perms::owner_write |
                                       std::filesystem::perms::group_read |
                                       std::filesystem::perms::others_read)
                                    : (std::filesystem::perms::owner_all |
                                       std::filesystem::perms::group_read |
                                       std::filesystem::perms::group_exec |
                                       std::filesystem::perms::others_read |
                                       std::filesystem::perms::others_exec);

    std::filesystem::permissions(p, perms,
                                 std::filesystem::perm_options::replace);
    return true;

  } else if (mode == "120000") {
    // Create symbolic link (data = target)
    try {
      std::filesystem::create_symlink(this->content, path);
    } catch (const std::exception &e) {
      std::cerr << "Failed to create symlink: " << e.what() << "\n";
      return false;
    }
    return true;
  }
}

bool Blob::deleteFile(const std::string &path) {
  std::cout << "deleting file " << path + this->name << "\n";
  std::filesystem::path p(path + this->name);
  if (std::filesystem::exists(p)) {
    bool deleted = std::filesystem::remove(p);
    if (!deleted) {
      std::cerr << "unable to delete file " << p.string() << "\n";
      return false;
    }
    return true;
  }
  return false;
}

bool Blob::overwriteFile(const std::string &path) {
  std::cout << "overwriting file " << path + this->name << "\n";
  std::filesystem::path p(path + this->name);
  if (!std::filesystem::exists(p)) {
    std::cout << "file not found for overwriting falling back to creation\n";
    this->createFile(path);
    return false;
  }
  std::ofstream out(p);
  if (!out) {
    std::cerr << "error opening file " << p.string() << std::endl;
    return false;
  }
  out << this->content;
  out.close();
  return true;
}

void TreeNode::loadFromDisk(const std::string &node_hash) {
  std::string content = Object::readFromDisk(node_hash);
  std::stringstream content_stream(content);
  std::string line;
  Config config;
  while (getline(content_stream, line)) {
    std::vector<std::string> line_content = config.split(line, ' ');
    std::string mode = line_content[0];
    std::string type = line_content[1];
    std::string hash = line_content[2];
    std::string name = line_content[3];
    if (type == "blob") {
      Blob *blob = new Blob();
      blob->loadFromDisk(hash);
      blob->hash = hash;
      blob->name = name;
      blob->mode = mode;
      this->children[name] = dynamic_cast<Object *>(blob);
    } else if (type == "tree") {
      TreeNode *treenode = new TreeNode();
      treenode->loadFromDisk(hash);
      treenode->hash = hash;
      treenode->name = name;
      treenode->mode = mode;
      this->children[name] = dynamic_cast<Object *>(treenode);
    }
  }
  this->content = content;
}
std::unordered_map<std::string, std::pair<std::string, std::string>>
Tree::flatten() {
  std::unordered_map<std::string, std::pair<std::string, std::string>>
      flattened_tree;
  flattenHelper(this->root, flattened_tree);
  return flattened_tree;
}

void Tree::flattenHelper(
    TreeNode *root,
    std::unordered_map<std::string, std::pair<std::string, std::string>>
        &entries) {
  if (!root) {
    return;
  }
  for (const auto &[name, obj] : root->children) {
    entries[name] = {obj->hash, obj->mode};
    if (TreeNode *treenode = dynamic_cast<TreeNode *>(obj)) {
      flattenHelper(treenode, entries);
    }
  }
}

void Tree::buildFromIndex() {
  Index idx;
  idx.load();
  auto blobs = idx.toMap();
  Config config;
  for (const auto &blob : blobs) {
    std::vector<std::string> path = config.split(blob.first, '/');
    std::string hash = blob.second.first;
    std::string mode = blob.second.second;
    insertBlob(root, path, 0, hash, mode);
  }
}

Tree::Tree() { root = new TreeNode(); }

void Tree::insertBlob(TreeNode *root, const std::vector<std::string> &path,
                      int level, const std::string &hash,
                      const std::string &mode) {

  if (level == path.size())
    return;

  std::string name = path[level];

  Object *obj = nullptr;

  if (level == path.size() - 1) {
    // Leaf → Blob
    obj = new Blob();
    obj->hash = hash;
    obj->mode = mode;
    obj->name = name;
    root->children[name] = obj; // key = filename
  } else {
    // Internal → TreeNode
    if (root->children.find(name) != root->children.end()) {
      // Already exists → recurse
      TreeNode *childNode = dynamic_cast<TreeNode *>(root->children[name]);
      if (childNode) {
        insertBlob(childNode, path, level + 1, hash, mode);
        return;
      }
    }

    // Create new intermediate tree node
    obj = new TreeNode();
    obj->name = name;
    root->children[name] = obj;

    // Recurse into the newly created tree node
    TreeNode *childNode = dynamic_cast<TreeNode *>(obj);
    insertBlob(childNode, path, level + 1, hash, mode);
  }
}

void Tree::loadTreeFromDisk(const std::string &hash) {
  this->root->loadFromDisk(hash);
}

void Tree::writeTreeToDisk(TreeNode *root) {
  std::ostringstream buffer;

  for (const auto &[name, obj] : root->children) {
    std::string type;
    if (Blob *blob = dynamic_cast<Blob *>(obj)) {
      // Entry: <mode> <type> <hash> <name>\n
      buffer << blob->mode << " " << "blob" << " " << blob->hash << " " << name
             << "\n";
    } else if (TreeNode *node = dynamic_cast<TreeNode *>(obj)) {
      // Recursively write subtree first to get its hash
      writeTreeToDisk(node);
      buffer << "040000" << " " << "tree" << " " << node->hash << " " << name
             << "\n";
    }
  }

  std::string raw_tree_content = buffer.str();
  root->content = "tree " + std::to_string(raw_tree_content.size()) +
                  std::string("\0", 1) + raw_tree_content;

  root->hash = root->getSha256();
  root->writeObjectToDisk();
}

Tree *Tree::write() {
  Tree *t = new Tree();
  t->buildFromIndex();
  t->writeTreeToDisk(t->root);
  return t;
}

void Tree::writeToWorkingDirectory(TreeNode *root, std::string path) {
  if (!root)
    return;
  for (const auto &[name, obj] : root->children) {
    if (Blob *blob = dynamic_cast<Blob *>(obj)) {
      blob->createFile(path);
    } else if (TreeNode *treenode = dynamic_cast<TreeNode *>(obj)) {
      Tree::writeToWorkingDirectory(treenode, path + treenode->name + "/");
    }
  }
}

void Tree::deleteFromWorkingDirectory(TreeNode *root, std::string path) {
  if (!root)
    return;
  for (const auto &[name, obj] : root->children) {
    if (Blob *blob = dynamic_cast<Blob *>(obj)) {
      bool removed = blob->deleteFile(path);
    } else if (TreeNode *treenode = dynamic_cast<TreeNode *>(obj)) {
      Tree::deleteFromWorkingDirectory(treenode, path + treenode->name + "/");
    }
  }
}

Commit::Commit() { this->tree = new Tree(); }

Commit::Commit(const std::string &message, const std::string &parent_hash) {
  Repository repo;
  Config config;
  config.load();
  Tree *t = new Tree();
  this->tree = t;
  this->tree->buildFromIndex();
  t->writeTreeToDisk(t->root);
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
  std::string timezone_offset = repo.getTimezoneOffset();

  std::ostringstream buffer;
  buffer << "tree " << t->root->hash << "\n";
  buffer << "parent " << parent_hash << "\n";
  buffer << "committer " << name << " <" << email << "> " << timestamp << " "
         << timezone_offset << "\n";

  buffer << message << "\n";
  std::string raw_content = buffer.str();
  this->content = "commit " + std::to_string(raw_content.size()) +
                  std::string("\0", 1) + raw_content;
  this->hash = this->getSha256();
}

Commit::Commit(const std::string &message, const std::string &parent1_hash,
               const std::string &parent2_hash) {
  Repository repo;
  Config config;
  config.load();
  Tree *t = new Tree();
  this->tree = t;
  this->tree->buildFromIndex();
  t->writeTreeToDisk(t->root);
  if (!config.hasUserConfig()) {
    std::cerr << "please provide the following values user.name and user.email "
                 "before committing.\n";
    exit(0);
  }
  std::string name = config.getUserName();
  std::string email = config.getUserEmail();
  long long timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::string timezone_offset = repo.getTimezoneOffset();

  std::ostringstream buffer;
  buffer << "tree " << t->root->hash << "\n";
  buffer << "parent " << parent1_hash << "\n";
  buffer << "parent " << parent2_hash << "\n";
  buffer << "committer " << name << " <" << email << "> " << timestamp << " "
         << timezone_offset << "\n";

  buffer << message << "\n";
  std::string raw_content = buffer.str();
  this->content = "commit " + std::to_string(raw_content.size()) +
                  std::string("\0", 1) + raw_content;
  this->hash = this->getSha256();
}

bool starts_with(std::string str, std::string prefix) {
  if (prefix.size() > str.size())
    return false;

  if (str.substr(0, prefix.size()) != prefix)
    return false;
  return true;
}

std::vector<std::string> split(std::string s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  for (char c : s) {
    if (c == delimiter) {
      tokens.push_back(token);
      token = "";
    } else {
      token.push_back(c);
    }
  }
  tokens.push_back(token);
  return tokens;
}

void Commit::loadFromDisk(const std::string &hash) {
  if (hash == NULL_HASH)
    return;
  std::string content = Object::readFromDisk(hash);
  std::stringstream content_stream(content);
  std::string line;
  getline(content_stream, line);
  Config config;
  std::vector<std::string> tree_info = config.split(line, ' ');

  parents.clear();
  line.clear();
  getline(content_stream, line);
  while (starts_with(line, "parent")) {
    std::vector<std::string> values = split(line, ' ');
    parents.push_back(values[1]);
    line.clear();
    getline(content_stream, line);
  }
  this->tree->root->loadFromDisk(tree_info[1]);
}

Commit *Commit::create(const std::string &treeHash,
                       const std::string &parent1Hash,
                       const std::string &parent2Hash,
                       const std::string &message) {
  Commit *commit = new Commit();

  commit->tree = new Tree();
  commit->tree->root->loadFromDisk(treeHash);

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
  std::string timezone_offset = repo.getTimezoneOffset();

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
  commit->content = "commit " + std::to_string(raw_content.size()) +
                    std::string("\0", 1) + raw_content;
  commit->hash = commit->getSha256();
  commit->writeObjectToDisk();

  return commit;
}

std::string Object::readFromDisk(const std::string &hash) {
  Repository repo;
  std::string object_dir = repo.objectsDir() + "/" + hash.substr(0, 2) + "/";
  std::string object_file = object_dir + hash.substr(2);

  if (!std::filesystem::exists(object_file)) {
    return "";
  }

  std::ifstream in(object_file, std::ios::binary);
  if (!in) {
    return "";
  }

  std::ostringstream buffer;
  buffer << in.rdbuf();
  std::string compressed = buffer.str();

  return decompressObject(compressed);
}

bool Object::isWrittenToDisk() {
  Repository repo;
  std::string object_dir =
      repo.objectsDir() + "/" + this->hash.substr(0, 2) + "/";
  if (!std::filesystem::exists(object_dir))
    return false;
  if (!std::filesystem::exists(object_dir + this->hash.substr(2)))
    return false;

  return true;
}

std::string Object::getSha256() {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, content.c_str(), content.size());
  SHA256_Final(hash, &sha256);

  // 4. Convert hash to hex string
  std::ostringstream hexStream;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return hexStream.str();
}
void Object::writeObjectToDisk() {
  Repository repo;
  std::string dirName = repo.objectsDir() + "/" + hash.substr(0, 2);
  std::string fileName = dirName + "/" + hash.substr(2);
  std::filesystem::create_directories(dirName);
  std::string compressed_object_content = compressObject(content);
  std::ofstream out(fileName, std::ios::binary);
  if (!compressed_object_content.empty()) {
    out.write(compressed_object_content.data(),
              static_cast<std::streamsize>(compressed_object_content.size()));
  } else {
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
  }
  out.close();
}

std::string Object::typeName() const {
  if (dynamic_cast<Blob *>(const_cast<Object *>(this)))
    return "blob";
  else if (dynamic_cast<TreeNode *>(const_cast<Object *>(this)))
    return "tree";
  else if (dynamic_cast<Commit *>(const_cast<Object *>(this)))
    return "commit";
  else
    return "object";
}

void Tree::diff(
    TreeNode *first, TreeNode *second,
    std::unordered_map<std::string,
                       std::vector<std::pair<Object *, std::string>>> &summary,
    const std::string &path) {
  if (!first && !second) {
    return;
  }

  if (first && !second) {
    if (first->typeName() == "blob") {
      summary["delete"].push_back({first, path});
    }
    return;
  }

  if (!first && second) {
    if (second->typeName() == "blob") {
      summary["create"].push_back({second, path});
    }
    return;
  }

  std::string first_type = first->typeName();
  std::string second_type = second->typeName();

  if (first_type == "blob" || second_type == "blob") {
    if (first->hash != second->hash) {
      if (second_type == "blob") {
        summary["overwrite"].push_back({second, path});
      }
    }
    return;
  }

  std::unordered_set<std::string> all_keys;
  for (auto &child : first->children) {
    all_keys.insert(child.first);
  }
  for (auto &child : second->children) {
    all_keys.insert(child.first);
  }

  for (auto &key : all_keys) {
    Object *first_child =
        (first->children.count(key) ? first->children[key] : nullptr);
    Object *second_child =
        (second->children.count(key) ? second->children[key] : nullptr);

    std::string new_path = path + key + "/";

    if (!first_child && second_child) {
      if (second_child->typeName() == "blob") {
        summary["create"].push_back({second_child, path});
      } else {
        TreeNode *tn_first = dynamic_cast<TreeNode *>(first_child);
        TreeNode *tn_second = dynamic_cast<TreeNode *>(second_child);
        diff(tn_first, tn_second, summary, new_path);
      }
    } else if (first_child && !second_child) {
      if (first_child->typeName() == "blob") {
        summary["delete"].push_back({first_child, path});
      } else {
        TreeNode *tn_first = dynamic_cast<TreeNode *>(first_child);
        TreeNode *tn_second = dynamic_cast<TreeNode *>(second_child);
        diff(tn_first, tn_second, summary, new_path);
      }
    } else {
      std::string ft = first_child->typeName();
      std::string st = second_child->typeName();

      if (ft == "blob" || st == "blob") {
        if (first_child->hash != second_child->hash) {
          if (st == "blob") {
            summary["overwrite"].push_back({second_child, path});
          }
        }
      } else {
        TreeNode *tn_first = dynamic_cast<TreeNode *>(first_child);
        TreeNode *tn_second = dynamic_cast<TreeNode *>(second_child);
        diff(tn_first, tn_second, summary, new_path);
      }
    }
  }
}

void Tree::buildFromDiff(
    std::unordered_map<std::string,
                       std::vector<std::pair<Object *, std::string>>> &diff) {
  Repository repo;
  std::string repo_root = repo.root();

  if (diff.find("delete") != diff.end()) {
    for (const auto &[obj, path] : diff["delete"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = repo_root + "/" + path;
      blob->deleteFile(full_path);
    }
  }

  if (diff.find("create") != diff.end()) {
    for (const auto &[obj, path] : diff["create"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = repo_root + "/" + path;
      blob->createFile(full_path);
    }
  }

  if (diff.find("overwrite") != diff.end()) {
    for (const auto &[obj, path] : diff["overwrite"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = repo_root + "/" + path;
      blob->overwriteFile(full_path);
    }
  }
}
