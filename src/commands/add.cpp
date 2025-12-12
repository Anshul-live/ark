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

int cmd_add(const std::vector<std::string>& args) {
    if(args.size() < 1){
            std::cout << "Usage:  add <filename>/<dirname> ..." << std::endl;
            return 1;
    }

    std::vector<std::string> paths;
    for(auto & path : args){
      paths.push_back(path);
    }

    std::filesystem::path repo_root = arkDir();                     
    std::filesystem::path index_path = repo_root / ".ark" / "index"; 
    std::unordered_set<std::string> ignored_patterns = loadIgnoreFiles();

    std::unordered_map<std::string, std::pair<std::string,std::string>> index = loadIndex();

    for (size_t i = 0; i < paths.size(); i++) {
        std::filesystem::path abs_path = std::filesystem::absolute(paths[i]);
        std::filesystem::path rel_path = std::filesystem::relative(abs_path, repo_root);

        std::string abs_norm  = normalizePath(abs_path.string());
        std::string rel_norm  = normalizePath(rel_path.string()); 

        if (isIgnored(abs_norm, ignored_patterns)) {
            continue;
        }

        if (!std::filesystem::exists(abs_path)) {
            std::cerr << abs_path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(abs_path)) {
            Blob* blob = hashObject(abs_path);
            std::string mode = getMode(abs_path);
            index[rel_norm] = { blob->hash, mode };
            blob->writeObjectToDisk();
        }
        else if (std::filesystem::is_directory(abs_path)) {
            if (isIgnored(abs_norm + "/", ignored_patterns)) {
                continue;
            }
            for (const auto& entry : std::filesystem::directory_iterator(abs_path)) {
                paths.push_back(entry.path().string()); 
            }
        }
    }

    std::ostringstream index_stream;
    for (auto& entry : index) {
        index_stream << entry.second.second 
                     << " "
                     << entry.second.first 
                     << " "
                     << entry.first         // relative path
                     << "\n";
    }

    std::ofstream out_file(index_path, std::ios::binary);
    if (!out_file) {
        std::cerr << "Failed to open index file for writing.\n";
        return 1;
    }
    out_file << index_stream.str();
    out_file.close();
    return 0;
}

