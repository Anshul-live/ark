#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <hash-object.h>
#include <compress.h>
#include <objects.h>

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
