#include <iostream>
#include <string>
#include <vector>

#include <add.h>
#include <ark.h>
#include <branch.h>
#include <cat-file.h>
#include <commit-tree.h>
#include <commit.h>
#include <exceptions.h>
#include <functional>
#include <hash-object.h>
#include <init.h>
#include <log.h>
#include <logger.h>
#include <repository.h>
#include <status.h>
#include <switch.h>
#include <unordered_map>
#include <update-ref.h>
#include <write-tree.h>

using CommandFn = std::function<int(const std::vector<std::string> &)>;

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cout << "Usage: " << argv[0] << " <command> [args...]\n";
      return 1;
    }

    std::string cmd = argv[1];

    if (cmd != "init") {
      try {
        Repository repo;
        repo.initialize();
      } catch (const NotARepositoryException &) {
      } catch (const std::exception &e) {
        LOG_EXCEPTION("main", e);
        return 0;
      }
    }

    std::unordered_map<std::string, CommandFn> commands = {
        {"init", cmd_init},
        {"hash-object", cmd_hashObject},
        {"cat-file", cmd_catFile},
        {"write-tree", cmd_writeTree},
        {"commit-tree", cmd_commitTree},
        {"update-ref", cmd_updateRef},
        {"add", cmd_add},
        {"branch", cmd_branch},
        {"switch", cmd_switch},
        {"status", cmd_status},
        {"log", cmd_log},
        {"commit", cmd_commit}};

    std::vector<std::string> args(argv + 2, argv + argc);

    auto it = commands.find(cmd);
    if (it == commands.end()) {
      std::cout << "Unknown command: " << cmd << "\n";
      return 1;
    }

    int result = it->second(args);

    if (cmd == "init" && result == 0) {
      try {
        Repository repo;
        repo.initialize();
      } catch (...) {
      }
    }

    return result;

  } catch (const NotARepositoryException &e) {
    std::cerr << e.what() << "\n";
    LOG_ERROR(std::string("Not a repository: ") + e.what());
    return 1;
  } catch (const BranchNotFoundException &e) {
    std::cerr << e.what() << "\n";
    LOG_ERROR(e.what());
    return 1;
  } catch (const BranchExistsException &e) {
    std::cerr << e.what() << "\n";
    LOG_ERROR(e.what());
    return 1;
  } catch (const ObjectNotFoundException &e) {
    std::cerr << e.what() << "\n";
    LOG_ERROR(e.what());
    return 1;
  } catch (const ArkException &e) {
    std::cerr << e.what() << "\n";
    LOG_ERROR(std::string("Ark error: ") + e.what());
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    LOG_FATAL(std::string("Unhandled exception: ") + e.what());
    return 1;
  }
}
