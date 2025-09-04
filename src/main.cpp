#include <iostream>
#include <init.h>
#include <hash-object.h>
#include <cat-file.h>

int main(int argc,char *argv[]){
    if(argc < 2){
        std::cout << "Usage: " << argv[0] << " <sub-command>" << std::endl;
        return 1;
    }
    std::string subCommand = argv[1];
    if(subCommand == "init"){
        init();
    }else if(subCommand == "hash-object"){
        if(argc < 3){
            std::cout << "Usage: " << argv[0] << " hash-object <filename>" << std::endl;
            return 1;
        }
        std::string filename = argv[2];
        hashObject(filename);
    }else if(subCommand == "cat-file"){
        if(argc < 3){
            std::cout << "Usage: " << argv[0] << " cat-file <object-hash>" << std::endl;
            return 1;
        }
        std::string objectHash = argv[2];
        catFile(objectHash);
    }
}