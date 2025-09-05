#ifndef ARK_HPP
#define ARK_HPP

#include <string>
#include <filesystem>

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
    std::cout<<"found root repo\n";
    static const std::string kArkDir = current / ".ark";
}


#endif

