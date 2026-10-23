#include <ark/repo/config.h>
#include <ark/repo/repository.h>
#include <ark/util/logger.h>
#include <ark/core/exceptions.h>
#include <ark/cli/commit_message_editor.h>
#include <ark/util/strings.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config::Config(const Repository& repo) : repo_(&repo) {}

void Config::load() {
    data.clear();
    LOG_INFO("Loading configuration");
    
    try {
        loadFromFile(repo_->configPath());
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
        std::string configPath = repo_->configPath();
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
        line = ark::text::trim(line);
        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line[0] == '[') {
            sectionName = ark::text::removeChars(line, "[]\"");
            sectionName = ark::text::collapseSpaces(sectionName);
            sectionName = ark::text::replaceChar(sectionName, ' ', '.');
            sectionName = ark::text::trim(sectionName);
        } else if (line[0] >= 'a' && line[0] <= 'z') {
            std::vector<std::string> content = ark::text::split(line, '=');
            if (content.size() != 2) {
                LOG_WARNING("Invalid config line format: " + line);
                continue;
            }
            std::string fieldName = ark::text::trim(content[0]);
            std::string value = ark::text::trim(ark::text::removeChars(content[1], "\""));
            data[sectionName][fieldName].push_back(value);
        }
    }
    in.close();
}
