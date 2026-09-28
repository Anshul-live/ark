#ifndef ARK_CLI_COMMANDS_H
#define ARK_CLI_COMMANDS_H

#include <string>
#include <vector>

int cmd_add(const std::vector<std::string> &args);
int cmd_branch(const std::vector<std::string> &args);
int cmd_catFile(const std::vector<std::string> &args);
int cmd_commitTree(const std::vector<std::string> &args);
int cmd_commit(const std::vector<std::string> &args);
int cmd_hashObject(const std::vector<std::string> &args);
int cmd_init(const std::vector<std::string> &args);
int cmd_log(const std::vector<std::string> &args);
int cmd_merge(const std::vector<std::string> &args);
int cmd_status(const std::vector<std::string> &args);
int cmd_switch(const std::vector<std::string> &args);
int cmd_updateRef(const std::vector<std::string> &args);
int cmd_writeTree(const std::vector<std::string> &args);

bool status();
void logBranch(const std::string &branchName);

#endif
