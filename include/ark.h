#ifndef ARK_HPP
#define ARK_HPP
#define NULL_HASH "0000000000000000000000000000000000000000000000000000000000000000"

#include <string>
#include <filesystem>
#include <sstream>
#include <unordered_set>
#include <objects.h>

std::unordered_set<std::string> loadIgnoreFile(const std::string& path);

bool isIndexSameAsCommit(const std::string& commit_hash);

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

std::string arkDir();


#endif

