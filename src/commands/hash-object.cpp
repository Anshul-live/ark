#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <hash-object.h>
#include <compress.h>

void hashObject(const std::string& filename) {
    std::cout << "hashing object" << std::endl;
    std::string arkPath = ".ark";

    // 1. Read file content
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: cannot open file " << filename << std::endl;
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();

    // 2. Create Git-like blob content (blob header + content)
    std::string objectContent = "blob " + std::to_string(fileContent.size()) + '\0' + fileContent;

    std::string compressedObjectContent = compressBuffer(objectContent);
    // 3. Compute SHA256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, objectContent.c_str(), objectContent.size());
    SHA256_Final(hash, &sha256);

    // 4. Convert hash to hex string
    std::ostringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string objectHash = hexStream.str();

    // 5. Build object storage path: .ark/objects/xx/yyyy...
    std::string dirName = arkPath + "/objects/" + objectHash.substr(0, 2);
    std::string fileName = dirName + "/" + objectHash.substr(2);

    std::filesystem::create_directories(dirName);

    // 6. Write object content to file
    std::ofstream out(fileName, std::ios::binary);
    if (!compressedObjectContent.empty()) {
        out.write(compressedObjectContent.data(), static_cast<std::streamsize>(compressedObjectContent.size()));
    } else {
        out.write(objectContent.data(), static_cast<std::streamsize>(objectContent.size()));
    }
    out.close();

    std::cout << "Object hashed: " << objectHash << std::endl;
}
