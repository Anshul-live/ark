#include <index.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <ark.h>

bool Index::contains(const std::string& path) const {
    return entries.find(path) != entries.end();
}

const IndexEntry* Index::get(const std::string& path) const {
    auto it = entries.find(path);
    return (it != entries.end()) ? &it->second : nullptr;
}

void Index::load() {
    entries.clear();

    std::string idxFile = arkDir() + "/.ark/index";
    std::ifstream in(idxFile, std::ios::binary);

    if (!in) {
        return;
    }

    std::string mode, hash, path;
    while (in >> mode >> hash >> path) {
        IndexEntry entry;
        entry.hash = hash;
        entry.mode = mode;
        entries[path] = entry;
    }
}

void Index::save() {
    std::string idxFile = arkDir() + "/.ark/index";
    std::ofstream out(idxFile, std::ios::binary);

    if (!out) {
        std::cerr << "Error: cannot write index file\n";
        return;
    }

    for (const auto& [path, entry] : entries) {
        out << entry.mode << " " << entry.hash << " " << path << "\n";
    }
}

void Index::stage(const std::string& path, const std::string& hash) {
    IndexEntry entry;
    entry.hash = hash;

    entry.mode = getMode(std::filesystem::path(path));

    entries[path] = entry;
}

void Index::unstage(const std::string& path) {
    entries.erase(path);
}

