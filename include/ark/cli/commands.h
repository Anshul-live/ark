#ifndef ARK_CLI_COMMANDS_H
#define ARK_CLI_COMMANDS_H

#include <string>
#include <vector>

#include <ark/repo/repository.h>

int cmd_add(const Repository &repo, const std::vector<std::string> &args);
int cmd_branch(const Repository &repo, const std::vector<std::string> &args);
int cmd_catFile(const Repository &repo, const std::vector<std::string> &args);
int cmd_commitTree(const Repository &repo, const std::vector<std::string> &args);
int cmd_commit(const Repository &repo, const std::vector<std::string> &args);
int cmd_hashObject(const Repository &repo, const std::vector<std::string> &args);
int cmd_init(const std::vector<std::string> &args);
int cmd_log(const Repository &repo, const std::vector<std::string> &args);
int cmd_merge(const Repository &repo, const std::vector<std::string> &args);
int cmd_status(const Repository &repo, const std::vector<std::string> &args);
int cmd_switch(const Repository &repo, const std::vector<std::string> &args);
int cmd_updateRef(const Repository &repo, const std::vector<std::string> &args);
int cmd_writeTree(const Repository &repo, const std::vector<std::string> &args);

#endif
