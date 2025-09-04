#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include <hash-object.h>
#include <compress.h>
#include <objects.h>

void hashObject(const std::string& filename) {
    if(!std::filesystem::is_regular_file(filename)){
        std::cerr<<"cannot hash given object not a normal file.\n";
        return;
    }
    Blob *blob = new Blob(filename);
    std::cout<<"hash: "<<blob->hash<<"\n";
    blob->writeObjectToDisk();
}
