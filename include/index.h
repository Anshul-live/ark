#pragma once
#include <string>
#include <unordered_map>

struct IndexEntry {
    std::string hash;  // blob hash
    std::string mode;  // "100644", "100755", "120000"
};

class Index {
public:
    std::unordered_map<std::string, IndexEntry> entries;

    void load();   // read .ark/index
    void save();   // write .ark/index

    void stage(const std::string& path, const std::string& hash);
    void unstage(const std::string& path);

    bool contains(const std::string& path) const;
    const IndexEntry* get(const std::string& path) const;
};

