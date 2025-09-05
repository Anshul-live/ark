#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark.h>
#include <init.h>

void init(){
    std::string ark_path = ".ark";
    if(std::filesystem::exists(ark_path)){
        std::cout << "ark already initialised" << std::endl;
        return;
    }
    std::filesystem::create_directory(ark_path);
    std::cout << "ark initialised" << std::endl;

    std::filesystem::create_directory(ark_path + "/objects");
    std::filesystem::create_directory(ark_path + "/refs");
    std::ofstream(ark_path + "/HEAD") << "ref: refs/heads/main";
}
