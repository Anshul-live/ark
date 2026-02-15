#include <config.h>
#include <repository.h>
#include <logger.h>
#include <exceptions.h>
#include <ark.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config::Config() {}

void Config::load() {
    data.clear();
    LOG_INFO("Loading configuration");
    
    try {
        Repository repo;
        loadFromFile(repo.configPath());
        loadFromFile("~/.arkconfig");
        loadFromFile("~/config/ark/config");
        LOG_INFO("Configuration loaded successfully");
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Config::load", e);
        throw ConfigException(std::string("Failed to load config: ") + e.what());
    }
}

void Config::save() {
    LOG_INFO("Saving configuration");
    
    try {
        Repository repo;
        std::string configPath = repo.configPath();
        std::ofstream out(configPath);
        if (!out) {
            LOG_ERROR("Failed to open config file for writing: " + configPath);
            throw ConfigException("Failed to open config file for writing");
        }
        for (const auto& [section, fields] : data) {
            out << "[" << section << "]\n";
            for (const auto& [key, values] : fields) {
                for (const auto& value : values) {
                    out << key << " = " << value << "\n";
                }
            }
        }
        out.close();
        LOG_INFO("Configuration saved successfully");
    } catch (const ConfigException& e) {
        LOG_EXCEPTION("Config::save", e);
        throw;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Config::save", e);
        throw ConfigException(std::string("Failed to save config: ") + e.what());
    }
}

std::string Config::getUserName() const {
    auto userIt = data.find("user");
    if (userIt != data.end()) {
        auto nameIt = userIt->second.find("name");
        if (nameIt != userIt->second.end() && !nameIt->second.empty()) {
            return nameIt->second[0];
        }
    }
    return "";
}

std::string Config::getUserEmail() const {
    auto userIt = data.find("user");
    if (userIt != data.end()) {
        auto emailIt = userIt->second.find("email");
        if (emailIt != userIt->second.end() && !emailIt->second.empty()) {
            return emailIt->second[0];
        }
    }
    return "";
}

void Config::setUserName(const std::string& name) {
    LOG_INFO("Setting user name: " + name);
    data["user"]["name"] = {name};
}

void Config::setUserEmail(const std::string& email) {
    LOG_INFO("Setting user email: " + email);
    data["user"]["email"] = {email};
}

bool Config::hasUserConfig() const {
    return !getUserName().empty() && !getUserEmail().empty();
}

void Config::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    
    std::string sectionName;
    std::string line;
    while (getline(in, line)) {
        line = this->trim(line);
        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line[0] == '[') {
            sectionName = this->removeCharacters(line, "[]\"");
            sectionName = this->removeExtraSpaces(sectionName);
            sectionName = this->replaceChar(sectionName, ' ', '.');
            sectionName = this->trim(sectionName);
        } else if (line[0] >= 'a' && line[0] <= 'z') {
            std::vector<std::string> content = this->split(line, '=');
            if (content.size() != 2) {
                LOG_WARNING("Invalid config line format: " + line);
                continue;
            }
            std::string fieldName = this->trim(content[0]);
            std::string value = this->trim(this->removeCharacters(content[1], "\""));
            data[sectionName][fieldName].push_back(value);
        }
    }
    in.close();
}

std::string Config::trim(const std::string& s) const {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Config::toLower(const std::string& s) const {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> Config::split(const std::string& s, char delimiter) const {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string Config::removeCharacters(const std::string& s, const std::string& chars) const {
    std::unordered_set<char> charSet(chars.begin(), chars.end());
    std::string result;
    for (char c : s) {
        if (!charSet.count(c)) {
            result += c;
        }
    }
    return result;
}

std::string Config::removeExtraSpaces(const std::string& s) const {
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

std::string Config::replaceChar(const std::string& s, char original, char replacement) const {
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
