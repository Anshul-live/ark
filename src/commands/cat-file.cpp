#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ark.h>
#include <cat-file.h>
#include <compress.h>

void catFile(const std::string& objectHash){
    std::string arkPath = arkDir();
    if(objectHash.size() < 3){
        std::cerr << "Error: invalid object hash" << std::endl;
        return;
    }

    std::string dirName = arkPath + "/objects/" + objectHash.substr(0,2);
    std::string fileName = dirName + "/" + objectHash.substr(2);

    if(!std::filesystem::exists(fileName)){
        std::cerr << "Error: object not found for hash " << objectHash << std::endl;
        return;
    }

    std::ifstream in(fileName, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string compressed = buffer.str();

    std::string data = decompressObject(compressed);
    if(data.empty()){
        std::cerr << "Error: failed to decompress object" << std::endl;
        return;
    }

    // Git-like blob: "blob <size>\0<content>"
    size_t nullPos = data.find('\0');
    if(nullPos == std::string::npos){
        std::cerr << "Error: invalid object format" << std::endl;
        return;
    }
    std::string header = data.substr(0, nullPos);
    std::string content = data.substr(nullPos + 1);

    if(header.rfind("blob ", 0) != 0){
        std::cerr << "Error: unsupported object type in header: " << header << std::endl;
        return;
    }

    std::cout << content;
}

