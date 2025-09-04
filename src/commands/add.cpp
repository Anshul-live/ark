#include <iostream>
#include <add.h>
#include <ark.h>
#include <filesystem>
#include <objects.h>
#include <unordered_map>
#include <hash-object.h>

void add(const std::vector<std::string>& paths) {
    std::filesystem::path repo_root = arkDir();               // repo root
    std::filesystem::path index_path = repo_root / ".ark" / "index";   // .ark/index

    std::unordered_map<std::string, std::string> index;

    for (const auto& path : paths) {
        if (path == ".") {
            // optional: add all files recursively later
            continue;
        }

        if (!std::filesystem::exists(path)) {
            std::cerr << path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(path)) {
            if (index.find(path) != index.end())
                continue;

            Blob* blob = hashObject(path);
            index[path] = blob->hash;
            blob->writeObjectToDisk();
        }
    }

    // Write index
    std::ostringstream index_stream;
    for (auto& entry : index) {
        std::string relativePath = std::filesystem::relative(entry.first, repo_root).string();
        index_stream << entry.second << " " << relativePath << "\n";
    }

    std::ofstream outFile(index_path, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open index file for writing.\n";
        return;
    }
    std::cout<<index_stream.str();
    outFile << index_stream.str();
    outFile.close();
}
