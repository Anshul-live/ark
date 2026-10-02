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
#include <ark/storage/object_format.h>
#include <ark/storage/object_store.h>
#include <ark/storage/sha256.h>
#include <ark/repo/repository.h>
#include <sstream>
#include <string>
#include <system_error>

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
  this->content = ark::frameObject(ark::ObjectType::Blob, file_content);

  this->hash = this->getSha256();
}

Blob *Blob::fromFile(const std::string &filename) { return new Blob(filename); }

void Blob::loadFromDisk(const std::string &hash) {
  std::string content = Object::readFromDisk(hash);
  this->content = Object::stripHeaderFromObject(hash, content);
}

bool Blob::createFile(const std::string &path) {
  std::cout << "creating file " << path << "\n";
  std::filesystem::path p(path);
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
  std::cout << "deleting file " << path << "\n";
  std::filesystem::path p(path);
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
  std::cout << "overwriting file " << path << "\n";
  std::filesystem::path p(path);
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
  std::string raw = Object::readFromDisk(node_hash);
  std::string content = Object::stripHeaderFromObject(node_hash, raw);
  std::stringstream content_stream(content);
  std::string line;
  while (getline(content_stream, line)) {
    std::vector<std::string> line_content = ark::text::split(line, ' ');
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
  this->content = raw;
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
  for (const auto &[repoPath, entry] : idx.entries()) {
    std::vector<std::string> path = ark::text::split(repoPath.str(), '/');
    std::string hash = entry.hash.hex();
    std::string mode(ark::toOctalString(entry.mode));
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
  root->content = ark::frameObject(ark::ObjectType::Tree, raw_tree_content);

  root->hash = root->getSha256();
  root->writeObjectToDisk();
}

Tree *Tree::write() {
  Tree *t = new Tree();
  t->buildFromIndex();
  t->writeTreeToDisk(t->root);
  return t;
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
  std::string timezone_offset = ark::formatLocalUtcOffset();

  std::ostringstream buffer;
  buffer << "tree " << t->root->hash << "\n";
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
  std::string timezone_offset = ark::formatLocalUtcOffset();

  std::ostringstream buffer;
  buffer << "tree " << t->root->hash << "\n";
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

  // both are trees
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

    std::string new_path = path + "/" + key;

    if (!first_child && second_child) {
      if (second_child->typeName() == "blob") {
        summary["create"].push_back({second_child, new_path});
      } else {
        diff(nullptr, dynamic_cast<TreeNode *>(second_child), summary,
             new_path);
      }

    } else if (first_child && !second_child) {
      if (first_child->typeName() == "blob") {
        summary["delete"].push_back({first_child, new_path});
      } else {
        diff(dynamic_cast<TreeNode *>(first_child), nullptr, summary, new_path);
      }

    } else {
      std::string ft = first_child->typeName();
      std::string st = second_child->typeName();

      if (ft == "blob" || st == "blob") {

        if (first_child->hash != second_child->hash) {
          if (st == "blob") {
            summary["overwrite"].push_back({second_child, new_path});
          }
        }
      } else {
        diff(dynamic_cast<TreeNode *>(first_child),
             dynamic_cast<TreeNode *>(second_child), summary, new_path);
      }
    }
  }
}
void Tree::buildFromDiff(
    std::unordered_map<std::string,
                       std::vector<std::pair<Object *, std::string>>> &diff) {
  Repository repo;
  // std::string repo_root = repo.root();

  if (diff.find("delete") != diff.end()) {
    for (const auto &[obj, path] : diff["delete"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = path;
      blob->deleteFile(full_path);
    }
  }

  if (diff.find("create") != diff.end()) {
    for (const auto &[obj, path] : diff["create"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = path;
      blob->createFile(full_path);
    }
  }

  if (diff.find("overwrite") != diff.end()) {
    for (const auto &[obj, path] : diff["overwrite"]) {
      Blob *blob = dynamic_cast<Blob *>(obj);
      std::string full_path = path;
      blob->overwriteFile(full_path);
    }
  }
}
