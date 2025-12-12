#include <status.h>
#include <objects.h>
#include <head.h>

bool status() {
    bool clean = true;

    std::unordered_map<std::string, Blob*> working_directory = loadWorkingDirectoryWithoutIgnored();
    std::unordered_map<std::string, std::pair<std::string, std::string>> index = loadIndex();
    std::string commit_hash = getHead();

    Commit* commit = new Commit();
    commit->loadFromDisk(commit_hash);
    std::unordered_map<std::string, std::pair<std::string,std::string>> latest_committed_files = commit->tree->flatten();

    std::unordered_set<std::string> printed;

    std::cout << "Changes to be committed:\n";
    for (const auto& [name, obj] : index) {
        const std::string& index_hash = obj.first;
        auto it = latest_committed_files.find(name);

        if (it == latest_committed_files.end() || it->second.first != index_hash) {
            std::cout << "\033[32m" << name << "\033[0m\n";
            printed.insert(name);
            clean = false;
        }
    }

    std::cout << "\nUntracked changes:\n";
    for (const auto& [name, blob] : working_directory) {
        if (index.find(name) == index.end() && latest_committed_files.find(name) == latest_committed_files.end()) {
            std::cout << "\033[31m" << name << " (untracked)\033[0m\n";
            clean = false;
        }
    }

    return clean;
}
