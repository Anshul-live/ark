#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark.h>
#include <init.h>

int cmd_init(const std::vector<std::string>& args){

    std::string ark_path = ".ark";
    if(std::filesystem::exists(ark_path)){
        std::cout << "ark already initialised" << std::endl;
        return 0;
    }
    std::filesystem::create_directory(ark_path);
    std::cout << "ark initialised" << std::endl;

    std::filesystem::create_directory(ark_path + "/objects");
    std::filesystem::create_directory(ark_path + "/refs");
    std::filesystem::create_directory(ark_path + "/refs/heads");
    std::ofstream(ark_path + "/refs/heads/main") << NULL_HASH;
    std::ofstream(ark_path + "/HEAD") << "ref: refs/heads/main";
    std::ofstream(ark_path + "/index");
    std::ofstream(ark_path + "/config");
    loadConfig();
    return 0;
}
