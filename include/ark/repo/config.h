#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_map>
#include <vector>

class Repository;

class Config {
public:
    explicit Config(const Repository& repo);
    void load();
    void save();

    std::string getUserName() const;
    std::string getUserEmail() const;
    void setUserName(const std::string& name);
    void setUserEmail(const std::string& email);

    bool hasUserConfig() const;

private:
    const Repository* repo_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> data;
    
    void loadFromFile(const std::string& path);
};

#endif
