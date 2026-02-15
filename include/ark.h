#ifndef ARK_HPP
#define ARK_HPP
#define NULL_HASH "0000000000000000000000000000000000000000000000000000000000000000"

#include <string>
#include <filesystem>

std::string getEditor();
void openEditor(const std::string& editor, const std::string& file);
std::string createTempFile();
std::string getCommitMessageFromEditor();
std::string readFileContent(const std::string& file);

#endif
