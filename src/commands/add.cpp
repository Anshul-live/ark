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

void add(std::vector<std::string>& paths) {
    std::filesystem::path repo_root = arkDir();                     // repo root
    std::filesystem::path index_path = repo_root / ".ark" / "index"; // .ark/index

    std::unordered_map<std::string, std::pair<std::string,std::string>> index; // path -> (hash, mode)

    for (int i = 0; i < paths.size(); i++) {
        std::filesystem::path path = std::filesystem::absolute(paths[i]);

        if (!std::filesystem::exists(path)) {
            std::cerr << path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(path)) {
            if (index.find(path.string()) != index.end())
                continue;

            Blob* blob = hashObject(path);
            std::string mode = getMode(path);
            index[path.string()] = { blob->hash, mode };
            blob->writeObjectToDisk();
        }
        else if (std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                paths.push_back(entry.path().string());
            }
        }
    }

    // Write index
    std::ostringstream index_stream;
    for (auto& entry : index) {
        std::string relative_path = std::filesystem::relative(entry.first, repo_root).string();
        index_stream << entry.second.second // mode
                     << " "
                     << entry.second.first  // hash
                     << " "
                     << relative_path
                     << "\n";
    }

    std::ofstream out_file(index_path, std::ios::binary);
    if (!out_file) {
        std::cerr << "Failed to open index file for writing.\n";
        return;
    }

    std::cout << index_stream.str();
    out_file << index_stream.str();
    out_file.close();
}

