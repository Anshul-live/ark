#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_map>
#include <vector>

class Config {
public:
    Config();
    void load();
    void save();

    std::string getUserName() const;
    std::string getUserEmail() const;
    void setUserName(const std::string& name);
    void setUserEmail(const std::string& email);

    bool hasUserConfig() const;

    std::string trim(const std::string& s) const;
    std::string toLower(const std::string& s) const;
    std::vector<std::string> split(const std::string& s, char delimiter) const;
    std::string removeCharacters(const std::string& s, const std::string& chars) const;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> data;
    
    void loadFromFile(const std::string& path);
    std::string removeExtraSpaces(const std::string& s) const;
    std::string replaceChar(const std::string& s, char original, char characterrr) const;
};

#endif
