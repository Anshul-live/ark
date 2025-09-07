#ifndef HEAD_HPP
#define HEAD_HPP

#include <string>

std::string getHead();
void setHead(const std::string& commit_hash);
std::string getBranchHash(const std::string& branch_name);
void updateHead(const std::string& branch_name);

#endif // !HEAD_HPP
