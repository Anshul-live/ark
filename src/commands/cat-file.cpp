#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ark.h>
#include <compress.h>

void catFile(const std::string& object_hash){
    std::string ark_path = arkDir();
    if(object_hash.size() < 3){
        std::cerr << "Error: invalid object hash" << std::endl;
        return;
    }

    std::string dirname = ark_path + "/.ark/objects/" + object_hash.substr(0,2);
    std::string filename = dirname + "/" + object_hash.substr(2);

    if(!std::filesystem::exists(filename)){
        std::cerr << "Error: object not found for hash " << object_hash << std::endl;
        return;
    }

    std::ifstream in(filename, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string compressed = buffer.str();

    std::string data = decompressObject(compressed);
    if(data.empty()){
        std::cerr << "Error: failed to decompress object" << std::endl;
        return;
    }

    // Git-like object: "<type> <size>\0<content>"
    size_t null_position = data.find('\0');
    if(null_position == std::string::npos){
        std::cerr << "Error: invalid object format" << std::endl;
        return;
    }

    std::string header = data.substr(0, null_position);
    std::string content = data.substr(null_position + 1);

    if(header.rfind("blob ", 0) == 0 || header.rfind("tree ",0) == 0 ||  header.rfind("commit ",0) == 0){
        std::cout << content;
    }
    else{
        std::cerr << "Error: unsupported object type in header: " << header << std::endl;
    }
}
