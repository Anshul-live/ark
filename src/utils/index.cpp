#include <index.h>
#include <ark.h>
#include <repository.h>
#include <logger.h>
#include <exceptions.h>
#include <fstream>
#include <iostream>
#include <filesystem>

Index::Index() {}

bool Index::contains(const std::string& path) const {
    return entries.find(path) != entries.end();
}

const IndexEntry* Index::get(const std::string& path) const {
    auto it = entries.find(path);
    return (it != entries.end()) ? &it->second : nullptr;
}

std::unordered_map<std::string, IndexEntry> Index::getEntries() const {
    return entries;
}

size_t Index::size() const {
    return entries.size();
}

void Index::clear() {
    entries.clear();
    LOG_DEBUG("Index cleared");
}

void Index::load() {
    entries.clear();
    LOG_DEBUG("Loading index");
    
    try {
        Repository repo;
        std::string idxFile = repo.indexPath();
        std::ifstream in(idxFile, std::ios::binary);
        
        if (!in) {
            LOG_DEBUG("No index file found, starting fresh");
            return;
        }

        std::string mode, hash, path;
        while (in >> mode >> hash >> path) {
            IndexEntry entry;
            entry.hash = hash;
            entry.mode = mode;
            entries[path] = entry;
        }
        in.close();
        
        LOG_INFO("Index loaded: " + std::to_string(entries.size()) + " entries");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Index::load", e);
        throw IndexException(std::string("Failed to load index: ") + e.what());
    }
}

void Index::save() {
    LOG_DEBUG("Saving index");
    
    try {
        Repository repo;
        std::string idxFile = repo.indexPath();
        std::ofstream out(idxFile, std::ios::binary);
        
        if (!out) {
            LOG_ERROR("Failed to open index file for writing: " + idxFile);
            throw IndexException("Failed to open index file for writing");
        }

        for (const auto& [path, entry] : entries) {
            out << entry.mode << " " << entry.hash << " " << path << "\n";
        }
        out.close();
        
        LOG_INFO("Index saved: " + std::to_string(entries.size()) + " entries");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Index::save", e);
        throw IndexException(std::string("Failed to save index: ") + e.what());
    }
}

void Index::stage(const std::string& path, const std::string& hash, const std::string& mode) {
    LOG_DEBUG("Staging file: " + path + " (hash: " + hash + ", mode: " + mode + ")");
    
    IndexEntry entry;
    entry.hash = hash;
    entry.mode = mode;
    entries[path] = entry;
}

void Index::unstage(const std::string& path) {
    LOG_DEBUG("Unstaging file: " + path);
    entries.erase(path);
}

void Index::remove(const std::string& path) {
    LOG_DEBUG("Removing from index: " + path);
    entries.erase(path);
}

void Index::setAll(const std::unordered_map<std::string, IndexEntry>& newEntries) {
    entries = newEntries;
}
