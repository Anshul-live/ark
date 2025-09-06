// represents a single file
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <iostream>
#include <ark.h>
#include <compress.h>
#include <objects.h>


Blob::Blob(const std::string& filename) {
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

Tree::Tree(const std::string& filename) {
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

 void Tree::insertBlob(TreeNode* root,
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
 

void Tree::writeTreeToDisk(TreeNode* root) {
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

Commit::Commit(const std::string& message,const std::string& parent_hash){
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


std::string Object::getSha256(){
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
    void Object::writeObjectToDisk(){
        std::string arkPath = arkDir() + "/.ark";
        std::string dirName = arkPath + "/objects/" + hash.substr(0, 2);
        std::string fileName = dirName + "/" + hash.substr(2);
        std::filesystem::create_directories(dirName);
        std::string compressed_object_content = compressObject(content);
        std::ofstream out(fileName, std::ios::binary);
    if (!compressed_object_content.empty()) {
        out.write(compressed_object_content.data(), static_cast<std::streamsize>(compressed_object_content.size()));
    } else {
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
        out.close();
}

    
