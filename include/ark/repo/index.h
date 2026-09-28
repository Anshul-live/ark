#pragma once
#include <string>
#include <unordered_map>
#include <memory>

struct IndexEntry {
    std::string hash;
    std::string mode;
};

class Index {
public:
    Index();
    
    void load();
    void save();

    void stage(const std::string& path, const std::string& hash, const std::string& mode);
    void unstage(const std::string& path);
    void remove(const std::string& path);
    void setAll(const std::unordered_map<std::string, IndexEntry>& newEntries);

    bool contains(const std::string& path) const;
    const IndexEntry* get(const std::string& path) const;
    
    std::unordered_map<std::string, IndexEntry> getEntries() const;
    size_t size() const;
    void clear();

    std::unordered_map<std::string, std::pair<std::string, std::string>> toMap() const;
    void fromMap(const std::unordered_map<std::string, std::pair<std::string, std::string>>& map);

private:
    std::unordered_map<std::string, IndexEntry> entries;
};

