#include <iostream>
#include <fstream>
#include <filesystem>
#include <ark/cli/commit_message_editor.h>
#include <ark/cli/commands.h>
#include <ark/core/object_hash.h>

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
    std::ofstream(ark_path + "/refs/heads/main") << ark::ObjectHash::zero().hex();
    std::ofstream(ark_path + "/HEAD") << "ref: refs/heads/main";
    std::ofstream(ark_path + "/index");
    std::ofstream(ark_path + "/config");
    return 0;
}
