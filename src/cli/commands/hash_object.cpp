#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <ark/cli/commands.h>
#include <ark/storage/compression.h>
#include <ark/objects/blob.h>
#include <ark/storage/object_store.h>

int cmd_hashObject(const Repository &, const std::vector<std::string> &args){
        if(args.size() < 1){
            std::cout << "Usage: hash-object <filename>" << std::endl;
            return 1;
        }
        std::string filename = args[0];
        auto blob = ark::Blob::tryReadFromFile(filename);
        if (!blob) {
            std::cout << "Error: cannot open file " << filename << std::endl;
            return 0;
        }
        std::cout << ark::ObjectStore::hashOf(ark::ObjectType::Blob, blob->data).hex();
        return 0;
}
