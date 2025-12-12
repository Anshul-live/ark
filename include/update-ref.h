#ifndef UPDATEREF_HPP
#define UPDATEREF_HPP

#include <fstream>


int cmd_updateRef(const std::vector<std::string> &args);

void updateRef(const std::string& ref_path,const std::string& hash);

#endif // !UPDATEREF_HPP
