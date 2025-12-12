#include <iostream>
#include <ark.h>
#include <index.h>
#include <filesystem>
#include <fstream>
#include <switch.h>
#include <head.h>
#include <objects.h>

int cmd_switch(const std::vector<std::string> &args) {
    if (args.size() < 1) {
        std::cout << "Usage: ark switch <branch-name>\n";
        return 1;
    }

    const std::string& branch_name = args[0];
    std::string repo_root = arkDir();
    std::string branch_path = repo_root + "/.ark/refs/heads/" + branch_name;

    if (!(std::filesystem::exists(branch_path) && std::filesystem::is_regular_file(branch_path))) {
        std::cerr << "branch " << branch_name << " does not exist\n";
        return 1;
    }

    std::string source_commit_hash = getHead();
    std::string target_commit_hash = getBranchHash(branch_name);
    std::string source_branch_name = getHeadBranchName();

    if (!isHeadDetached() && source_branch_name == branch_name) {
        std::cerr << "already on branch " << branch_name << "\n";
        return 0;
    }

    Commit source_commit;
    source_commit.loadFromDisk(source_commit_hash);

    Commit target_commit;
    target_commit.loadFromDisk(target_commit_hash);

    std::unordered_map<std::string, std::vector<std::pair<Object*, std::string>>> diff;
    treeDiff(source_commit.tree->root, target_commit.tree->root, diff, "");

    updateHead(branch_name);
    buildWorkingDirectoryFromTreeDiff(diff);

    Index idx;
    idx.entries.clear();

    auto flat = target_commit.tree->flatten();
    for (auto& [path, pair] : flat) {
        IndexEntry entry;
        entry.hash = pair.first;
        entry.mode = pair.second;
        idx.entries[path] = entry;
    }

    idx.save();

    std::cout << "switched branch to " << branch_name << "\n";
    return 0;
}

