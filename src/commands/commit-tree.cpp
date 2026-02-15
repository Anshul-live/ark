#include <commit-tree.h>
#include <repository.h>
#include <objects.h>

int cmd_commitTree(const std::vector<std::string> &args){
  int argc = args.size();
    if(argc < 1){
            std::cout << "Usage: commit-tree <tree-hash> [<parent1-hash>] [<parent2-hash>]" << std::endl;
            return 1;
        }
        std::string parent1_hash = argc > 1 ? args[2] : "";
        std::string parent2_hash = argc > 2 ? args[3] : "";
        std::string message = argc > 3 ? args[4] : "";
        
        Commit* commit = Commit::create(args[0], parent1_hash, parent2_hash, message);
        std::cout << commit->hash;
        return 0;
}
