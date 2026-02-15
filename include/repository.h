#ifndef REPOSITORY_HPP
#define REPOSITORY_HPP

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>

class Blob;

class Repository {
public:
    Repository();
    ~Repository();
    
    static void initialize();
    static void initialize(const std::string& logPath);
    
    std::string root() const;
    std::string arkPath() const;
    std::string objectsDir() const;
    std::string indexPath() const;
    std::string configPath() const;
    std::string headPath() const;
    std::string refsDir() const;
    std::string logPath() const;

    bool isValid() const;
    static bool exists(const std::string& path);

    std::string normalizePath(const std::string& path) const;
    std::string getMode(const std::filesystem::path& path) const;

    std::unordered_set<std::string> loadIgnorePatterns() const;
    bool isIgnored(const std::string& path) const;

    std::string getTimezoneOffset() const;

    std::unordered_map<std::string, Blob*> loadWorkingDirectory() const;
    std::unordered_map<std::string, Blob*> loadWorkingDirectoryWithoutIgnored() const;

private:
    std::string findRoot() const;
    mutable std::string cachedRoot;
    mutable bool rootCached;
};

#endif
