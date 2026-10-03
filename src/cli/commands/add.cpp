#include <iostream>
#include <ark/cli/commands.h>
#include <ark/cli/commit_message_editor.h>
#include <filesystem>
#include <ark/objects/blob.h>
#include <ark/objects/objects.h>
#include <ark/storage/object_store.h>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <ark/repo/repository.h>
#include <ark/repo/index.h>
#include <ark/util/fs.h>

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

        std::string abs_norm  = ark::normalizeGenericPath(abs_path.string());
        std::string rel_norm  = ark::normalizeGenericPath(rel_path.string()); 

        if (repo.isIgnored(abs_norm)) {
            continue;
        }

        if (!std::filesystem::exists(abs_path)) {
            std::cerr << abs_path << " does not exist.\n";
            continue;
        }

        if (std::filesystem::is_regular_file(abs_path)) {
            ark::Blob blob = ark::Blob::readFromFile(abs_path);
            if (!std::ifstream(abs_path, std::ios::binary)) {
                return 1;
            }
            std::string mode = repo.getMode(abs_path);
            ark::ObjectHash blobHash = ark::ObjectStore::hashOf(ark::ObjectType::Blob, blob.data);
            ark::FileEntry entry{blobHash,
                                 ark::parseFileMode(mode).value_or(ark::FileMode::Regular)};
            idx.stage(ark::RepoPath::fromRelative(rel_norm), entry);
            ark::ObjectStore(repo.objectsDir()).write(ark::ObjectType::Blob, blob.data);
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
