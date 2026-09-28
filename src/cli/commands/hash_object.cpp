#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <ark/cli/commands.h>
#include <ark/storage/compression.h>
#include <ark/objects/objects.h>

int cmd_hashObject(const std::vector<std::string> &args){
        if(args.size() < 1){
            std::cout << "Usage: hash-object <filename>" << std::endl;
            return 1;
        }
        std::string filename = args[0];
        Blob* blob = Blob::fromFile(filename);
        if(!blob){
          return 1;
        }
        std::cout << blob->hash;
        return 0;
}
