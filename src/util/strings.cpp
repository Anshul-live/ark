#include <algorithm>
#include <ark/util/strings.h>
#include <sstream>
#include <unordered_set>

namespace ark::text {

std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::string> splitKeepEmpty(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  for (char c : s) {
    if (c == delimiter) {
      tokens.push_back(token);
      token = "";
    } else {
      token.push_back(c);
    }
  }
  tokens.push_back(token);
  return tokens;
}

bool startsWith(const std::string &str, const std::string &prefix) {
  if (prefix.size() > str.size())
    return false;

  if (str.substr(0, prefix.size()) != prefix)
    return false;
  return true;
}

std::string removeChars(const std::string &s, const std::string &chars) {
  std::unordered_set<char> charSet(chars.begin(), chars.end());
  std::string result;
  for (char c : s) {
    if (!charSet.count(c)) {
      result += c;
    }
  }
  return result;
}

std::string collapseSpaces(const std::string &s) {
  std::string result;
  int count = 0;
  for (size_t i = 0; i < s.length(); i++) {
    char el = s[i];
    if (el == ' ') {
      if (count <= 1) {
        result.push_back(el);
        count++;
      }
      continue;
    }
    count = 0;
    result.push_back(el);
  }
  return result;
}

std::string replaceChar(const std::string &s, char original, char replacement) {
  std::string result;
  for (char c : s) {
    if (c == original) {
      result.push_back(replacement);
    } else {
      result.push_back(c);
    }
  }
  return result;
}

} // namespace ark::text
