#ifndef ARK_STORAGE_SHA256_H
#define ARK_STORAGE_SHA256_H

#include <array>
#include <string>
#include <string_view>

namespace ark {

std::array<unsigned char, 32> sha256Digest(std::string_view data);
std::string sha256Hex(std::string_view data);

} // namespace ark

#endif
