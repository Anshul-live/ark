#include <iostream>
#include <add.h>
#include <ark.h>
#include <filesystem>
#include <objects.h>
#include <unordered_map>
#include <hash-object.h>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <repository.h>
#include <index.h>

int cmd_add(const std::vector<std::string>& args) {
    if(args.size() < 1){
            std::cout << "Usage:  add <filename>/<dirname> ..." << std::endl;
            return 1;
    }

    std::vector<std::string> paths;
    for(auto & path : args){
      paths.push_back(path);
    }

    Repository repo;
    Index idx;
    idx.load();

    for (size_t i = 0; i < paths.size(); i++) {
        std::filesystem::path abs_path = std::filesystem::absolute(paths[i]);
        std::filesystem::path rel_path = std::filesystem::relative(abs_path, repo.root());

        std::string abs_norm  = repo.normalizePath(abs_path.string());
        std::string rel_norm  = repo.normalizePath(rel_path.string()); 

        if (repo.isIgnored(abs_norm)) {
            continue;
        }

        if (!std::filesystem::exists(abs_path)) {
            std::cerr << abs_path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(abs_path)) {
            Blob* blob = Blob::fromFile(abs_path);
            std::string mode = repo.getMode(abs_path);
            idx.stage(rel_norm, blob->hash, mode);
            blob->writeObjectToDisk();
        }
        else if (std::filesystem::is_directory(abs_path)) {
            if (repo.isIgnored(abs_norm + "/")) {
                continue;
            }
            for (const auto& entry : std::filesystem::directory_iterator(abs_path)) {
                paths.push_back(entry.path().string()); 
            }
        }
    }

    idx.save();
    return 0;
}
