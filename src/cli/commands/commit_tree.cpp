#include <ark/cli/commands.h>
#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/objects/flat_tree.h>
#include <ark/repo/config.h>
#include <ark/repo/repository.h>
#include <ark/storage/object_store.h>
#include <iostream>

int cmd_commitTree(const std::vector<std::string> &args){
  int argc = args.size();
    if(argc < 1){
            std::cout << "Usage: commit-tree <tree-hash> [<parent1-hash>] [<parent2-hash>]" << std::endl;
            return 1;
        }
        std::string parent1_hash = argc > 1 ? args[2] : "";
        std::string parent2_hash = argc > 2 ? args[3] : "";
        std::string message = argc > 3 ? args[4] : "";

        Repository repo;
        ark::ObjectStore store(repo.objectsDir());
        ark::ObjectHash tree = ark::ObjectHash::unchecked(args[0]);
        ark::flattenTree(store, tree);

        Config config;
        config.load();
        if (!config.hasUserConfig()) {
          std::cerr << "please provide user.name and user.email before committing.\n";
          exit(0);
        }

        ark::Commit commit;
        commit.tree = tree;
        if (!parent1_hash.empty())
          commit.parents.push_back(ark::ObjectHash::unchecked(parent1_hash));
        if (!parent2_hash.empty())
          commit.parents.push_back(ark::ObjectHash::unchecked(parent2_hash));
        commit.committer = ark::Signature::now(
            ark::Identity{config.getUserName(), config.getUserEmail()});
        commit.message = message;

        ark::CommitStore commits(store);
        std::cout << commits.write(commit).hex();
        return 0;
}
