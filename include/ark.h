#ifndef ARK_HPP
#define ARK_HPP

#include <string>

inline const std::string& arkDir(){
    static const std::string kArkDir = ".ark";
    return kArkDir;
}

#endif

