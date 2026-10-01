#include <ark/repo/index.h>
#include <ark/repo/index_file.h>
#include <ark/repo/repository.h>
#include <ark/util/logger.h>
#include <ark/core/exceptions.h>
#include <fstream>

Index::Index() {}

bool Index::contains(const ark::RepoPath& path) const {
    return entries_.find(path) != entries_.end();
}

const ark::FileEntry* Index::find(const ark::RepoPath& path) const {
    auto it = entries_.find(path);
    return (it != entries_.end()) ? &it->second : nullptr;
}

const ark::FlatTree& Index::entries() const {
    return entries_;
}

std::size_t Index::size() const {
    return entries_.size();
}

void Index::clear() {
    entries_.clear();
    LOG_DEBUG("Index cleared");
}

void Index::load() {
    entries_.clear();
    LOG_DEBUG("Loading index");

    try {
        Repository repo;
        std::string idxFile = repo.indexPath();
        std::ifstream in(idxFile, std::ios::binary);

        if (!in) {
            LOG_DEBUG("No index file found, starting fresh");
            return;
        }

        entries_ = ark::IndexFile::parse(in);

        LOG_INFO("Index loaded: " + std::to_string(entries_.size()) + " entries");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Index::load", e);
        throw IndexException(std::string("Failed to load index: ") + e.what());
    }
}

void Index::save() const {
    LOG_DEBUG("Saving index");

    try {
        Repository repo;
        std::string idxFile = repo.indexPath();
        std::ofstream out(idxFile, std::ios::binary);

        if (!out) {
            LOG_ERROR("Failed to open index file for writing: " + idxFile);
            throw IndexException("Failed to open index file for writing");
        }

        ark::IndexFile::write(out, entries_);
        out.close();

        LOG_INFO("Index saved: " + std::to_string(entries_.size()) + " entries");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Index::save", e);
        throw IndexException(std::string("Failed to save index: ") + e.what());
    }
}

void Index::stage(const ark::RepoPath& path, ark::FileEntry entry) {
    LOG_DEBUG("Staging file: " + path.str() + " (hash: " + entry.hash.hex() +
              ", mode: " + std::string(ark::toOctalString(entry.mode)) + ")");
    entries_.insert_or_assign(path, entry);
}

void Index::erase(const ark::RepoPath& path) {
    LOG_DEBUG("Removing from index: " + path.str());
    entries_.erase(path);
}

void Index::replaceAll(ark::FlatTree newEntries) {
    entries_ = std::move(newEntries);
}

void Index::stageAll(const ark::FlatTree& newEntries) {
    for (const auto& [path, entry] : newEntries) {
        entries_.insert_or_assign(path, entry);
    }
}
