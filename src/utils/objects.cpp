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

    std::string Object::getSha256(){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::string arkPath = ".ark";
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
        std::string arkPath = arkDir();
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
    