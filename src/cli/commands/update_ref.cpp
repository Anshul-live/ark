#include <ark/cli/commands.h>
#include <ark/cli/commit_message_editor.h>
#include <ark/repo/ref_store.h>
#include <iostream>

int cmd_updateRef(const std::vector<std::string> &args){
  if(args.size() < 2){
            std::cout << "Usage: update-ref <ref-path> <commit-hash>" << std::endl;
            return 1;
  }
  RefStore ref;
  ref.writeRef(ark::RefName::unchecked(args[0]), ark::ObjectHash::unchecked(args[1]));
  return 0;
}
