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

    virtual ~Object() = default;
    Object() = default;

    std::string getSha256();
    void writeObjectToDisk();
    bool isWrittenToDisk();

    std::string typeName() const;

    static std::string readFromDisk(const std::string& hash);
};

// ---------- Blob ----------
class Blob : public Object {
public:
    Blob() = default;

    Blob(const std::string& filename);
    static Blob* fromFile(const std::string& filename);
    void loadFromDisk(const std::string& hash);
    void writeToWorkingTree();
    bool createFile(const std::string& path);
    bool deleteFile(const std::string& path);
    bool overwriteFile(const std::string& path);
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
    void buildFromIndex();
    void loadTreeFromDisk(const std::string& hash);
    void writeToWorkingDirectory(TreeNode* root,std::string path);
    void deleteFromWorkingDirectory(TreeNode* root,std::string path);
    std::unordered_map<std::string,std::pair<std::string,std::string>> flatten();
    void flattenHelper(TreeNode* root,std::unordered_map<std::string,std::pair<std::string,std::string>>& entries);

    static Tree* write();
    static void diff(TreeNode* first, TreeNode* second,
                     std::unordered_map<std::string, std::vector<std::pair<Object*, std::string>>>& summary,
                     const std::string& path);
    static void buildFromDiff(std::unordered_map<std::string, std::vector<std::pair<Object*, std::string>>>& diff);
};

class Commit:public Object{
  public:
    Tree* tree;
    Commit();
    Commit(const std::string& message,const std::string& parent_hash);
    Commit(const std::string& message,const std::string& parent1_hash,const std::string& parent2_hash);
    void loadFromDisk(const std::string& hash);

    static Commit* create(const std::string& treeHash, const std::string& parent1Hash, 
                         const std::string& parent2Hash, const std::string& message);
};

#endif

