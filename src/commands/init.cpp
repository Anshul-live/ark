#include <iostream>
#include <fstream>
#include <filesystem>
#include <init.h>

void init(){
    std::cout << "initialising ark" << std::endl;
    std::string arkPath = ".ark";
    if(std::filesystem::exists(arkPath)){
        std::cout << "ark already initialised" << std::endl;
        return;
    }
    std::filesystem::create_directory(arkPath);
    std::cout << "ark initialised" << std::endl;

    std::filesystem::create_directory(arkPath + "/objects");
    std::filesystem::create_directory(arkPath + "/refs");
    std::ofstream(arkPath + "/HEAD") << "ref: refs/heads/main";
}