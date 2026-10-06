#include <iostream>
#include <ark/cli/commands.h>
#include <ark/cli/commit_message_editor.h>
#include <filesystem>
#include <ark/objects/blob.h>
#include <ark/storage/object_store.h>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <ark/repo/repository.h>
#include <ark/repo/ignore_rules.h>
#include <ark/repo/index.h>
#include <ark/repo/working_tree.h>
#include <ark/core/file_mode.h>
#include <ark/util/fs.h>

int cmd_add(const Repository& repo, const std::vector<std::string>& args) {
    if(args.size() < 1){
            std::cout << "Usage:  add <filename>/<dirname> ..." << std::endl;
            return 1;
    }

    std::vector<std::string> paths;
    for(auto & path : args){
      paths.push_back(path);
    }

    ark::IgnoreRules ignore = ark::IgnoreRules::load(repo);
    ark::WorkingTree tree(repo, ignore);
    Index idx(repo);
    idx.load();

    for (const std::string& path : paths) {
        bool completed = tree.forEachFile(
            std::filesystem::absolute(path),
            [&](const std::filesystem::path& abs_path) {
                std::filesystem::path rel_path = std::filesystem::relative(abs_path, repo.root());
                std::string rel_norm = ark::normalizeGenericPath(rel_path.string());

                ark::Blob blob = ark::Blob::readFromFile(abs_path);
                if (!std::ifstream(abs_path, std::ios::binary)) {
                    return false;
                }
                ark::FileMode mode = ark::detectFileMode(abs_path).value_or(ark::FileMode::Regular);
                ark::ObjectHash blobHash = ark::ObjectStore::hashOf(ark::ObjectType::Blob, blob.data);
                idx.stage(ark::RepoPath::fromRelative(rel_norm), ark::FileEntry{blobHash, mode});
                ark::ObjectStore(repo.objectsDir()).write(ark::ObjectType::Blob, blob.data);
                return true;
            },
            [](const std::filesystem::path& missing) {
                std::cerr << missing << " does not exist.\n";
            });
        if (!completed) {
            return 1;
        }
    }

    idx.save();
    return 0;
}
