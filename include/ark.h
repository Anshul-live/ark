#ifndef ARK_HPP
#define ARK_HPP

#define NULL_HASH "0000000000000000000000000000000000000000000000000000000000000000"

#include <string>
#include <filesystem>
#include <sstream>
#include <unordered_set>

std::unordered_set<std::string> loadIgnoreFile(const std::string& path);

std::unordered_set<std::string> loadIgnoreFiles();

std::string normalizePath(const std::string& p);

bool isIgnored(const std::string& pattern,const std::unordered_set<std::string>& ignored_patterns);

std::string trim(const std::string& s);

std::vector<std::string> split(const std::string& s, char delimiter);


std::string getMode(const std::filesystem::path& path);

std::string getEditor();

void openEditor(const std::string& editor, const std::string& file);

std::string createTempFile();

std::string getCommitMessageFromEditor();

std::string readFileContent(const std::string& file);

std::unordered_map<std::string,std::string> loadConfig();



inline const std::string arkDir(){
    std::filesystem::path current = std::filesystem::current_path();

    while (true) {
        if (std::filesystem::exists(current / ".ark") && std::filesystem::is_directory(current / ".ark")) {
            return current; // found repo root
        }

        if (current.has_parent_path() && current.string() != "/") {
            current = current.parent_path();
        } else {
            throw std::runtime_error("Not inside an Ark repository.");
        }
    }
    static const std::string kArkDir = current / ".ark";
}


#endif

