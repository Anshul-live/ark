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

    Blob(const std::string& filename);
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
  Tree() = default;
    TreeNode* root;

    Tree(const std::string& filename);

 void insertBlob(TreeNode* root,
                const std::vector<std::string>& path,
                int level,
                const std::string& hash,
                const std::string& mode);
 

void writeTreeToDisk(TreeNode* root);

};

class Commit:public Object{
  public:
    Commit() = default;
    Commit(const std::string& message,const std::string& parent_hash);
};
// ---------- Object base ----------


#endif

