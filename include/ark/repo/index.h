#pragma once
#include <ark/core/repo_path.h>
#include <ark/objects/file_entry.h>
#include <ark/objects/flat_tree.h>
#include <cstddef>

class Index {
public:
    Index();

    void load();
    void save() const;

    void stage(const ark::RepoPath& path, ark::FileEntry entry);
    void replaceAll(ark::FlatTree newEntries);
    void stageAll(const ark::FlatTree& newEntries);
    void erase(const ark::RepoPath& path);

    bool contains(const ark::RepoPath& path) const;
    const ark::FileEntry* find(const ark::RepoPath& path) const;

    const ark::FlatTree& entries() const;
    std::size_t size() const;
    void clear();

private:
    ark::FlatTree entries_;
};
