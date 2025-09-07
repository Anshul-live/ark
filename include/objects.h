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
    bool isWrittenToDisk();
};

// ---------- Blob ----------
class Blob : public Object {
public:
    Blob() = default;

    Blob(const std::string& filename);
    void loadFromDisk(const std::string& hash);
};

// ---------- TreeNode ----------
class TreeNode : public Object {
public:
    // Key = hash, Value = child object
    std::unordered_map<std::string, Object*> children;
    void loadFromDisk(const std::string& hash);
};

// ---------- Tree ----------
class Tree {
public:
    TreeNode* root;

    Tree();

 void insertBlob(TreeNode* root,
                const std::vector<std::string>& path,
                int level,
                const std::string& hash,
                const std::string& mode);
 

void writeTreeToDisk(TreeNode* root);
void loadTreeFromDisk(const std::string& hash);
void buildWorkingDirectoryFromTree(TreeNode* root,std::string path);
};

class Commit:public Object{
  public:
    Tree* tree;
    Commit();
    Commit(const std::string& message,const std::string& parent_hash);
    Commit(const std::string& message,const std::string& parent1_hash,const std::string& parent2_hash);
    void loadFromDisk(const std::string& hash);
};
// ---------- Object base ----------

std::unordered_map<std::string,Blob*> loadIndexAsBlobs();

std::unordered_map<std::string,std::pair<std::string,std::string>> loadIndex();

std::unordered_map<std::string,Blob*> loadWorkingDirectory();

std::unordered_map<std::string,Blob*> loadWorkingDirectoryWithoutIgnored();


#endif

