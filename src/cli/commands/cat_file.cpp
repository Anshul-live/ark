#include <ark/cli/commit_message_editor.h>
#include <ark/cli/commands.h>
#include <ark/storage/compression.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ark/core/exceptions.h>
#include <ark/objects/objects.h>
#include <ark/storage/object_store.h>
#include <ark/repo/repository.h>
#include <sstream>

int cmd_catFile(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    std::cout << "Usage: cat-file <object-hash>" << std::endl;
    return 1;
  }
  std::string object_hash = args[0];

  if (object_hash.size() < 3) {
    std::cerr << "Error: invalid object hash" << std::endl;
    return 1;
  }

  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  ark::ObjectHash hash = ark::ObjectHash::unchecked(object_hash);

  std::string data = store.readFramed(hash).value_or("");
  if (data.empty()) {
    std::cerr << "Error: object not found for hash " << object_hash
              << std::endl;
    return 1;
  }

  try {
    std::cout << ark::parseObjectFrame(hash, data).body << "\n";
    return 0;
  } catch (const ObjectFormatException &) {
    // Lenient fallback keeps the historic output for frames whose declared
    // size does not match the body.
  }

  size_t null_position = data.find('\0');
  if (null_position == std::string::npos) {
    std::cerr << "Error: invalid object format" << std::endl;
    return 1;
  }

  std::string header = data.substr(0, null_position);
  if (header.rfind("blob ", 0) == 0 || header.rfind("tree ", 0) == 0 ||
      header.rfind("commit ", 0) == 0) {
    std::cout << data.substr(null_position + 1) << "\n";
    return 0;
  }
  std::cerr << "Error: unsupported object type in header: " << header
            << std::endl;
  return 1;
}
