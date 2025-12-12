#include <iostream>
#include <vector>
#include <string>

#include <init.h>
#include <hash-object.h>
#include <cat-file.h>
#include <add.h>
#include <commit.h>
#include <branch.h>
#include <switch.h>
#include <log.h>
#include <status.h>
#include <write-tree.h>
#include <commit-tree.h>
#include <update-ref.h>
#include <ark.h>
#include <unordered_map>
#include <functional>

using CommandFn = std::function<int(const std::vector<std::string>&)>;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <command> [args...]\n";
        return 1;
    }

    std::unordered_map<std::string, CommandFn> commands = {
        {"init",        cmd_init},
        {"hash-object", cmd_hashObject},
        {"cat-file",    cmd_catFile},
        {"write-tree",  cmd_writeTree},
        {"commit-tree", cmd_commitTree},
        {"update-ref",  cmd_updateRef},
        {"add",         cmd_add},
        {"branch",      cmd_branch},
        {"switch",      cmd_switch},
        {"status",      cmd_status},
        {"log",         cmd_log},
        {"commit",      cmd_commit}
    };

    std::string cmd = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);

    auto it = commands.find(cmd);
    if (it == commands.end()) {
        std::cout << "Unknown command: " << cmd << "\n";
        return 1;
    }

    return it->second(args);
}

