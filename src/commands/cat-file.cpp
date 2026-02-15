#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ark.h>
#include <compress.h>
#include <cat-file.h>
#include <repository.h>
#include <objects.h>

int cmd_catFile(const std::vector<std::string> &args){
  if(args.size() < 1){
      std::cout << "Usage: cat-file <object-hash>" << std::endl;
      return 1;
  } 
  std::string object_hash = args[0];
  
  if(object_hash.size() < 3){
      std::cerr << "Error: invalid object hash" << std::endl;
      return 1;
  }

  std::string data = Object::readFromDisk(object_hash);
  if(data.empty()){
      std::cerr << "Error: object not found for hash " << object_hash << std::endl;
      return 1;
  }

  size_t null_position = data.find('\0');
  if(null_position == std::string::npos){
      std::cerr << "Error: invalid object format" << std::endl;
      return 1;
  }

  std::string header = data.substr(0, null_position);
  std::string content = data.substr(null_position + 1);

  if(header.rfind("blob ", 0) == 0 || header.rfind("tree ",0) == 0 ||  header.rfind("commit ",0) == 0){
      std::cout << content << "\n";
      return 0;
  }
  else{
      std::cerr << "Error: unsupported object type in header: " << header << std::endl;
  }
  return 1;
}
