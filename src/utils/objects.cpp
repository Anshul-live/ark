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
#include <hash-object.h>
#include <cat-file.h>

std::unordered_map<std::string,Blob*> loadIndexAsBlobs(){
  //TODO: implement me

}

std::unordered_map<std::string,std::pair<std::string,std::string>> loadIndex(){
  std::string filename = arkDir() + "/.ark/index";
  std::cout<<filename<<"\n";
      std::ifstream in(filename, std::ios::binary);
        if (!in) {
            std::cerr << "provided index file does not exist or can't be opened";
            return {};
        }

        std::unordered_map<std::string, std::pair<std::string, std::string>> blobs;
        std::string mode, hash, name;

        while (in >> mode >> hash >> name) {
            blobs[name] = {hash, mode};
        }
        return blobs;
}

std::unordered_map<std::string,Blob*> loadWorkingDirectoryWithoutIgnored(){
    std::filesystem::path repo_root = arkDir();                     // repo root
    std::unordered_set<std::string> ignored_patterns = loadIgnoreFiles();
    std::unordered_map<std::string,Blob*> blobs;
    std::vector<std::string> paths;
    paths.push_back(repo_root);
    for (int i = 0; i < paths.size(); i++) {
        std::filesystem::path path = std::filesystem::absolute(paths[i]);
        std::string generic_path = normalizePath(path);

        if(isIgnored(generic_path,ignored_patterns)){
            continue;
        }

        if (!std::filesystem::exists(path)) {
            std::cerr << path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(path)) {
            Blob* blob = hashObject(path);
            std::string mode = getMode(path);
            blobs[std::filesystem::relative(path,repo_root).string()] = blob;
        }
        else if (std::filesystem::is_directory(path)) {
          if(isIgnored(generic_path+"/",ignored_patterns)){
            continue;
          }
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                paths.push_back(entry.path().string());
            }
        }
    }
    return blobs;
}


std::unordered_map<std::string,Blob*> loadWorkingDirectory() {
    std::filesystem::path repo_root = arkDir();                     
    std::vector<std::string> paths;
    paths.push_back(repo_root);
    std::unordered_map<std::string,Blob*> blobs;

    for (int i = 0; i < paths.size(); i++) {
        std::string path = paths[i];
        path = std::filesystem::relative(path,repo_root);
        if (std::filesystem::is_regular_file(path)) {
            Blob* blob = hashObject(path);
            std::string mode = getMode(path);
            blobs[path] = blob;
        }
        else if (std::filesystem::is_directory(path)) {
          if(path == ".ark")
            continue;
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                paths.push_back(entry.path().string());
            }
        }
    }
    return blobs;
}


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

void Blob::loadFromDisk(const std::string& hash){
  std::string content = catFile(hash);
  this->content = content;
}

bool Blob::createFile(const std::string& path) {
    std::filesystem::path p(path+this->name);
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
            auto perms = (mode == "100644")
                ? (std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
                   std::filesystem::perms::group_read | std::filesystem::perms::others_read)
                : (std::filesystem::perms::owner_all |
                   std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                   std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

            std::filesystem::permissions(p, perms,
                                         std::filesystem::perm_options::replace);
            return true;

        } else if (mode == "120000") {
            // Create symbolic link (data = target)
            try {
                std::filesystem::create_symlink(this->content, path);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create symlink: " << e.what() << "\n";
                return false;
            }
            return true;

        } 
}

bool Blob::deleteFile(const std::string& path){
  std::filesystem::path p(path+this->name); 
  if(std::filesystem::exists(p)){
    bool deleted = std::filesystem::remove(p);
    if(!deleted){
      std::cerr<<"unable to delete file "<<p.string()<<"\n";
      return false;
    }
    return true;
  }
  return false;
}

void TreeNode::loadFromDisk(const std::string& hash){
  std::string content = catFile(hash);
  std::stringstream content_stream(content);
  std::string line;
  while(getline(content_stream,line)){
    std::vector<std::string> line_content = split(line,' ');
    std::string mode = line_content[0];
    std::string type = line_content[1];
    std::string hash = line_content[2];
    std::string name = line_content[3];

    if(type == "blob"){
      Blob * blob = new Blob();
      blob->loadFromDisk(hash);
      blob->hash = hash;
      blob->name = name;
      blob->mode = mode;
      this->children[name] = dynamic_cast<Object*>(blob);
    }
    else if(type == "tree"){
      TreeNode* treenode = new TreeNode();
      treenode->loadFromDisk(hash);
      treenode->hash = hash;
      treenode->name = name;
      treenode->mode = mode;
      this->children[name] = dynamic_cast<Object*>(treenode);
    }
  }
  this->content = content;
}
std::unordered_map<std::string,std::string> Tree::flatten(){
  std::unordered_map<std::string,std::string> flattened_tree;
  flattenHelper(this->root,flattened_tree);
  return flattened_tree;
}

void Tree::flattenHelper(TreeNode* root,std::unordered_map<std::string,std::string>& entries){
  if(!root){
    return;
  }
  for(const auto& [name,obj]:root->children){
    entries[name] = obj->hash;
    if(TreeNode* treenode = dynamic_cast<TreeNode*>(obj)){
      flattenHelper(treenode,entries);
    }
  }
}

Tree::Tree() {
  std::unordered_map<std::string,std::pair<std::string,std::string>> blobs = loadIndex();

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

void Tree::loadTreeFromDisk(const std::string& hash){
  //TODO: implement this
  this->root->loadFromDisk(hash);
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


void Tree::writeToWorkingDirectory(TreeNode* root,std::string path){
  if(!root)
    return;
  for(const auto& [name,obj]:root->children){
    if(Blob* blob = dynamic_cast<Blob*> (obj)){
      blob->createFile(path);
    }
    else if(TreeNode* treenode = dynamic_cast<TreeNode*> (obj)){
      Tree::writeToWorkingDirectory(treenode,path+treenode->name+"/");
    }
  }
}

void Tree::deleteFromWorkingDirectory(TreeNode* root,std::string path){
  if(!root)
    return;
  for(const auto& [name,obj]:root->children){
    if(Blob* blob = dynamic_cast<Blob*> (obj)){
      bool removed = blob->deleteFile(path);
    }
    else if(TreeNode* treenode = dynamic_cast<TreeNode*> (obj)){
      Tree::deleteFromWorkingDirectory(treenode,path+treenode->name+"/");
    }
  }
}

Commit::Commit(){
  this->tree = new Tree();
}

Commit::Commit(const std::string& message,const std::string& parent_hash){
      std::string ark_path = arkDir();
      Tree * t = new Tree();
      this->tree = t;
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

Commit::Commit(const std::string& message,const std::string& parent1_hash,const std::string& parent2_hash){
      std::string ark_path = arkDir();
      Tree * t = new Tree();
      this->tree = t;
      t->writeTreeToDisk(t->root);

      // TODO: Implement properly

      std::ostringstream buffer;
      buffer << "tree "<< t->root->hash << "\n";
      buffer << "parent "<<parent1_hash<<"\n";
      buffer << "parent "<<parent2_hash<<"\n";
      buffer << "author "<<"\n";
      buffer << "committer"<<"\n\n";

      buffer<<message<<"\n";
      std::string raw_content = buffer.str();
      this->content = "commit "+std::to_string(raw_content.size()) + std::string("\0",1)+raw_content;
      this->hash = this->getSha256();
}

void Commit::loadFromDisk(const std::string& hash){
  if(hash == NULL_HASH)
    return;

  std::string content = catFile(hash);
  std::stringstream content_stream(content);
  std::string line;
  getline(content_stream,line);
  std::vector<std::string> tree_info = split(line,' ');
  this->tree->root->loadFromDisk(tree_info[1]);
}

bool Object::isWrittenToDisk(){
  std::string repo_root = arkDir();
  std::string object_dir = repo_root+"/.ark/objects/"+this->hash.substr(0,2)+"/";
  if(!std::filesystem::exists(object_dir))
    return false;
  if(!std::filesystem::exists(object_dir+this->hash.substr(2)))
    return false;

  return true;
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

    
