#include <ark/core/object_hash.h>

namespace ark {

std::optional<ObjectHash> ObjectHash::parse(std::string_view hex) {
  if (hex.size() != kHexLength)
    return std::nullopt;
  for (char c : hex) {
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!ok)
      return std::nullopt;
  }
  return ObjectHash(std::string(hex));
}

ObjectHash ObjectHash::fromDigest(const std::array<unsigned char, 32> &digest) {
  static const char *digits = "0123456789abcdef";
  std::string out;
  out.reserve(kHexLength);
  for (unsigned char b : digest) {
    out.push_back(digits[b >> 4]);
    out.push_back(digits[b & 0x0f]);
  }
  return ObjectHash(std::move(out));
}

ObjectHash ObjectHash::zero() { return ObjectHash(std::string(kHexLength, '0')); }

bool ObjectHash::isZero() const {
  return hex_.find_first_not_of('0') == std::string::npos;
}

std::string_view ObjectHash::fanoutDir() const {
  return std::string_view(hex_).substr(0, 2);
}

std::string_view ObjectHash::fanoutFile() const {
  return std::string_view(hex_).substr(2);
}

std::ostream &operator<<(std::ostream &os, const ObjectHash &hash) {
  return os << hash.hex();
}

} // namespace ark
