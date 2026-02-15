#include <repository.h>
#include <logger.h>
#include <exceptions.h>
#include <ark.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <iostream>

Repository::Repository() : rootCached(false) {}

Repository::~Repository() {}

void Repository::initialize() {
    initialize("");
}

void Repository::initialize(const std::string& logPath) {
    try {
        std::string path = logPath.empty() ? ".ark/ark.log" : logPath;
        Logger::instance().init(path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
    }
}

std::string Repository::root() const {
    if (!rootCached) {
        try {
            cachedRoot = findRoot();
            rootCached = true;
        } catch (const std::exception& e) {
            LOG_EXCEPTION("Repository::root", e);
            throw NotARepositoryException();
        }
    }
    return cachedRoot;
}

std::string Repository::findRoot() const {
    std::filesystem::path current = std::filesystem::current_path();
    
    while (true) {
        if (std::filesystem::exists(current / ".ark") && 
            std::filesystem::is_directory(current / ".ark")) {
            return current.string();
        }
        
        if (current.has_parent_path() && current.string() != "/") {
            current = current.parent_path();
        } else {
            LOG_ERROR("Not inside an Ark repository");
            throw NotARepositoryException(current.string());
        }
    }
}

bool Repository::isValid() const {
    try {
        std::string r = root();
        return std::filesystem::exists(r + "/.ark");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Repository::isValid", e);
        return false;
    }
}

bool Repository::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string Repository::objectsDir() const {
    try {
        return root() + "/.ark/objects";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::objectsDir", e);
        throw;
    }
}

std::string Repository::indexPath() const {
    try {
        return root() + "/.ark/index";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::indexPath", e);
        throw;
    }
}

std::string Repository::configPath() const {
    try {
        return root() + "/.ark/config";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::configPath", e);
        throw;
    }
}

std::string Repository::headPath() const {
    try {
        return root() + "/.ark/HEAD";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::headPath", e);
        throw;
    }
}

std::string Repository::refsDir() const {
    try {
        return root() + "/.ark/refs";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::refsDir", e);
        throw;
    }
}

std::string Repository::logPath() const {
    try {
        return root() + "/.ark/ark.log";
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::logPath", e);
        throw;
    }
}

std::string Repository::normalizePath(const std::string& path) const {
    return std::filesystem::path(path).lexically_normal().generic_string();
}

std::string Repository::getMode(const std::filesystem::path& path) const {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        LOG_WARNING("Failed to get mode for: " + path.string());
        return "0";
    }
    
    if (S_ISREG(st.st_mode)) {
        if (st.st_mode & S_IXUSR) return "100755";
        else return "100644";
    } else if (S_ISDIR(st.st_mode)) {
        return "040000";
    } else if (S_ISLNK(st.st_mode)) {
        return "120000";
    }
    return "0";
}

std::unordered_set<std::string> Repository::loadIgnorePatterns() const {
    std::unordered_set<std::string> patterns;
    std::vector<std::string> paths;
    
    try {
        paths.push_back(root());
    } catch (const NotARepositoryException& e) {
        LOG_EXCEPTION("Repository::loadIgnorePatterns", e);
        return patterns;
    }
    
    for (size_t i = 0; i < paths.size(); i++) {
        std::string path = paths[i];
        std::string ignoreFile = path + "/.arkignore";
        
        if (std::filesystem::exists(ignoreFile) && 
            std::filesystem::is_regular_file(ignoreFile)) {
            std::ifstream in(ignoreFile);
            std::string line;
            while (std::getline(in, line)) {
                line = trim(line);
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                patterns.insert(normalizePath(path + "/" + line));
            }
            in.close();
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (std::filesystem::is_directory(entry)) {
                    paths.push_back(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Error reading directory: " + path);
        }
    }
    return patterns;
}

bool Repository::isIgnored(const std::string& path) const {
    static std::unordered_set<std::string> cachedPatterns;
    
    if (cachedPatterns.empty()) {
        try {
            cachedPatterns = loadIgnorePatterns();
        } catch (const std::exception& e) {
            LOG_EXCEPTION("Repository::isIgnored", e);
            return false;
        }
    }
    
    std::string normalizedPath = normalizePath(path);
    
    for (const auto& pattern : cachedPatterns) {
        if (normalizedPath == pattern) {
            return true;
        }
        if (!pattern.empty() && pattern.back() == '/') {
            if (normalizedPath.rfind(pattern, 0) == 0) {
                return true;
            }
        }
    }
    return false;
}
