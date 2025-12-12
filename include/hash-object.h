#ifndef HASH_OBJECT_HPP
#define HASH_OBJECT_HPP

#include <string>
#include <objects.h>

Blob* hashObject(const std::string& filename);

int cmd_hashObject(const std::vector<std::string> &args);

#endif
