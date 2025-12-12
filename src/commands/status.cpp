#include <status.h>
#include <objects.h>
#include <head.h>
#include <index.h>

int cmd_status(const std::vector<std::string> &args){
  status();
  return 0;
}

bool status() {
    bool clean = true;

    auto working_directory = loadWorkingDirectoryWithoutIgnored();

    Index idx;
    idx.load();

    std::string commit_hash = getHead();
    Commit commit;
    commit.loadFromDisk(commit_hash);
    auto latest_committed_files = commit.tree->flatten();

    std::unordered_set<std::string> printed;

    std::cout << "Changes to be committed:\n";

    for (const auto& [path, entry] : idx.entries) {
        const std::string& index_hash = entry.hash;

        auto it = latest_committed_files.find(path);

        if (it == latest_committed_files.end() ||
            it->second.first != index_hash) {

            std::cout << "\033[32m" << path << "\033[0m\n";
            printed.insert(path);
            clean = false;
        }
    }

    std::cout << "\nChanges not staged for commit:\n";

    for (const auto& [path, blob] : working_directory) {
        const IndexEntry* ent = idx.get(path);
        if (ent) {
            // fresh compute working-tree hash
            Blob fresh(path);

            if (fresh.hash != ent->hash &&
                printed.find(path) == printed.end()) {

                std::cout << "\033[31m" << path << " (modified)\033[0m\n";
                printed.insert(path);
                clean = false;
            }
        }
    }

    std::cout << "\nUntracked files:\n";

    for (const auto& [path, blob] : working_directory) {
        bool not_in_index  = !idx.contains(path);
        bool not_in_commit = latest_committed_files.find(path) == latest_committed_files.end();

        if (not_in_index && not_in_commit &&
            printed.find(path) == printed.end()) {

            std::cout << "\033[31m" << path << " (untracked)\033[0m\n";
            clean = false;
        }
    }

    return clean;
}

