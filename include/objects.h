#ifndef OBJECTS_HPP
#define OBJECTS_HPP

#include <fstream>
#include <string>
#include <sstream>
#include <openssl/sha.h>
#include <compress.h>

class Object{
    public:
        std::string hash;
        std::string content;
        Object(){
        }
        std::string getSha256();
    void writeObjectToDisk();
};

class Blob: public Object{
    public:
Blob(const std::string& filename){
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
        std::cout << "Error: cannot open file " << filename << std::endl;
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string file_content = buffer.str();

    // 2. Create Git-like blob content (blob header + content)
    this->content = "blob " + std::to_string(file_content.size()) + '\0' + file_content;
    this->hash = this->getSha256();
    }
};

class Tree:public Object{
    public:
    Tree(const std::string& dirname){

    }

};

#endif