#include <iostream>
#include <add.h>
#include <ark.h>
#include <filesystem>
#include <objects.h>
#include <unordered_map>
#include <hash-object.h>

void add(std::vector<std::string>& paths) {
    std::filesystem::path repo_root = arkDir();               // repo root
    std::filesystem::path index_path = repo_root / ".ark" / "index";   // .ark/index

    std::unordered_map<std::string, std::string> index;

    for (int i = 0;i < paths.size();i++) {
        std::cout<<paths[i]<<"<--- raw path\n";
        std::filesystem::path path = std::filesystem::absolute(paths[i]);

        if (!std::filesystem::exists(path)) {
            std::cerr << path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(path)) {
            std::cout<<"processing file: "<<path<<"\n";
            if (index.find(path) != index.end())
                continue;

            Blob* blob = hashObject(path);
            index[path] = blob->hash;
            blob->writeObjectToDisk();
        }
        else if(std::filesystem::is_directory(path)){
          std::cout<<"processing dir -> "<<path<<std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                std::string temp_path = entry.path().string();
                std::cout<<"new entry-> "<<temp_path<<std::endl;
                paths.push_back(temp_path);
            }
        }
        
    }

    // Write index
    std::ostringstream index_stream;
    for (auto& entry : index) {
        std::string relative_path = std::filesystem::relative(entry.first, repo_root).string();
        index_stream << entry.second << " " << relative_path << "\n";
    }

    std::ofstream out_file(index_path, std::ios::binary);
    if (!out_file) {
        std::cerr << "Failed to open index file for writing.\n";
        return;
    }
    std::cout<<index_stream.str();
    out_file << index_stream.str();
    out_file.close();
}
