#include <commit-tree.h>

int cmd_commitTree(const std::vector<std::string> &args){
  int argc = args.size();
    if(argc < 1){
            std::cout << "Usage: commit-tree <tree-hash> [<parent1-hash>] [<parent2-hash>]" << std::endl;
            return 1;
        }
        std::string parent1_hash = argc > 1 ? args[2] : "";
        std::string parent2_hash = argc > 2 ? args[3] : "";
        std::string message = argc > 3 ? args[4] : "";
        std::cout<<commitTree(args[0],parent1_hash,parent2_hash,message);
}

std::string commitTree(const std::string& tree_hash,const std::string& parent1_hash,const std::string& parent2_hash,const std::string& message){
  Commit* commit = new Commit();
  commit->tree->buildFromIndex();
  std::string ark_path = arkDir();
  std::string index_file_path = ark_path + "/.ark/index";

      std::ostringstream buffer;
      buffer << "tree "<< tree_hash << "\n";
      if(parent1_hash != "")
        buffer << "parent "<<parent1_hash<<"\n";
      else
        buffer << "parent "<<NULL_HASH<<"\n";
      if(parent2_hash != "")
        buffer << "parent "<<parent2_hash<<"\n";
      buffer << "author "<<"\n";
      buffer << "committer"<<"\n\n";

      buffer<<message<<"\n";
      std::string raw_content = buffer.str();
      commit->content = "commit "+std::to_string(raw_content.size()) + std::string("\0",1)+raw_content;
      commit->hash = commit->getSha256();
      std::ofstream out(index_file_path);
      out.close();
      commit->writeObjectToDisk();
      return commit->hash;
}
