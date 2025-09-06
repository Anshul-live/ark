#include <iostream>
#include <vector>
#include <string>

#include <init.h>
#include <hash-object.h>
#include <cat-file.h>
#include <add.h>
#include <commit.h>
#include <branch.h>
#include <switch.h>
#include <log.h>

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
        Blob* blob = hashObject(filename);
        std::cout<<blob->hash<<"\n";
    }else if(subCommand == "cat-file"){
        if(argc < 3){
            std::cout << "Usage: " << argv[0] << " cat-file <object-hash>" << std::endl;
            return 1;
        }
        std::string objectHash = argv[2];
        std::cout<<catFile(objectHash);
    }
    else if(subCommand == "add"){
        if(argc < 3){
            std::cout << "Usage: " << argv[0] << " add <filename>/<dirname> ..." << std::endl;
            return 1;
        }
        std::vector<std::string> paths;
        for(int i = 2;i < argc;i++){
            std::string path(argv[i]);
            paths.push_back(path);
        }
        add(paths);
    }
    else if(subCommand == "branch"){
        if(argc < 3){
            printBranches();
            return 1;
        }
        branch(argv[2]);
    }
    else if(subCommand == "switch"){
        if(argc < 3){
            std::cout << "Usage: " << argv[0] << " add <filename>/<dirname> ..." << std::endl;
            return 1;
        }
        switchBranch(argv[2]);
    }
    else if(subCommand == "log"){
      log();
    }
    else if(subCommand == "commit"){
      commit();
    }
}
