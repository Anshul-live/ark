#ifndef OBJECTS_HPP
#define OBJECTS_HPP

#include <fstream>
#include <string>
#include <sstream>
#include <openssl/sha.h>
#include <compress.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <ark.h>

std::vector<std::string> split(const std::string& s, char delimiter);
// ---------- Object base ----------
class Object {
public:
    std::string mode;
    std::string hash;
    std::string content;
    std::string name;

    virtual ~Object() = default;   // ensure proper cleanup
    Object() = default;

    std::string getSha256();
    void writeObjectToDisk();
};

// ---------- Blob ----------
class Blob : public Object {
public:
    Blob() = default;

    Blob(const std::string& filename) {
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
};

// ---------- TreeNode ----------
class TreeNode : public Object {
public:
    // Key = hash, Value = child object
    std::unordered_map<std::string, Object*> children;
};

// ---------- Tree ----------
class Tree {
public:
    TreeNode* root;

    Tree(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) {
            std::cerr << "provided index file does not exist or can't be opened";
            root = nullptr;
            return;
        }

        std::unordered_map<std::string, std::pair<std::string, std::string>> blobs;
        std::string mode, hash, name;

        while (in >> mode >> hash >> name) {
            blobs[name] = {hash, mode};
        }

        root = new TreeNode();

        for (const auto& blob : blobs) {
            std::vector<std::string> path = split(blob.first, '/');
            std::string hash = blob.second.first;
            std::string mode = blob.second.second;
            insertBlob(root, path, 0, hash, mode);
        }
    }

 void insertBlob(TreeNode* root,
                const std::vector<std::string>& path,
                int level,
                const std::string& hash,
                const std::string& mode) {

    if (level == path.size())
        return;

    std::string name = path[level];

    Object* obj = nullptr;

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
            TreeNode* childNode = dynamic_cast<TreeNode*>(root->children[name]);
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
        TreeNode* childNode = dynamic_cast<TreeNode*>(obj);
        insertBlob(childNode, path, level + 1, hash, mode);
    }
}
 

void writeTreeToDisk(TreeNode* root) {
    std::ostringstream buffer;

    for (const auto& [name,obj] : root->children) {
    std::string type;
    if (Blob* blob = dynamic_cast<Blob*>(obj)) {
        // Entry: <mode> <type> <hash> <name>\n
        buffer << blob->mode << " " << "blob" << " " << blob->hash << " " << name << "\n";
    }
    else if (TreeNode* node = dynamic_cast<TreeNode*>(obj)) {
        // Recursively write subtree first to get its hash
        writeTreeToDisk(node);
        buffer << "040000" << " " << "tree" << " " << node->hash << " " << name << "\n";
    }
    }
    

    std::string raw_tree_content = buffer.str();
    root->content = "tree " + std::to_string(raw_tree_content.size()) + std::string("\0", 1) + raw_tree_content;

    root->hash = root->getSha256();
    root->writeObjectToDisk();
}

};

class Commit:public Object{
  public:
    Commit(const std::string& message,const std::string& parent_hash){
      std::string ark_path = arkDir();
      std::string index_file_path = ark_path + "/.ark/index";
      Tree * t = new Tree(index_file_path);
      t->writeTreeToDisk(t->root);

      // TODO: Implement properly

      std::ostringstream buffer;
      buffer << "tree "<< t->root->hash << "\n";
      buffer << "parent "<<parent_hash<<"\n";
      buffer << "author "<<"\n";
      buffer << "committer"<<"\n\n";

      buffer<<message<<"\n";
      std::string raw_content = buffer.str();
      this->content = "commit "+std::to_string(raw_content.size()) + std::string("\0",1)+raw_content;
      this->hash = this->getSha256();
    }
};

#endif

