#include <update-ref.h>
#include <ark.h>
#include <ref.h>
#include <iostream>

int cmd_updateRef(const std::vector<std::string> &args){
  if(args.size() < 2){
            std::cout << "Usage: update-ref <ref-path> <commit-hash>" << std::endl;
            return 1;
  }
  Ref ref;
  ref.updateRef(args[0], args[1]);
  return 0;
}
