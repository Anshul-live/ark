#ifndef ARK_UTIL_STRINGS_H
#define ARK_UTIL_STRINGS_H

#include <string>
#include <vector>

namespace ark::text {

std::string trim(const std::string &s);
std::string toLower(const std::string &s);

// Drops a trailing empty token.
std::vector<std::string> split(const std::string &s, char delimiter);
// Keeps empty tokens, including a trailing one.
std::vector<std::string> splitKeepEmpty(const std::string &s, char delimiter);

bool startsWith(const std::string &str, const std::string &prefix);
std::string removeChars(const std::string &s, const std::string &chars);
std::string collapseSpaces(const std::string &s);
std::string replaceChar(const std::string &s, char original, char replacement);

} // namespace ark::text

#endif
