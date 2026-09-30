#ifndef ARK_CORE_OBJECT_HASH_H
#define ARK_CORE_OBJECT_HASH_H

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace ark {

// A SHA-256 object id held as 64 lowercase hex characters.
class ObjectHash {
public:
  static constexpr std::size_t kHexLength = 64;

  static std::optional<ObjectHash> parse(std::string_view hex);
  static ObjectHash fromDigest(const std::array<unsigned char, 32> &digest);
  static ObjectHash zero();

  bool isZero() const;
  const std::string &hex() const { return hex_; }
  std::string_view fanoutDir() const;
  std::string_view fanoutFile() const;

  friend bool operator==(const ObjectHash &a, const ObjectHash &b) {
    return a.hex_ == b.hex_;
  }
  friend bool operator!=(const ObjectHash &a, const ObjectHash &b) {
    return a.hex_ != b.hex_;
  }
  friend bool operator<(const ObjectHash &a, const ObjectHash &b) {
    return a.hex_ < b.hex_;
  }

private:
  explicit ObjectHash(std::string hex) : hex_(std::move(hex)) {}
  std::string hex_;
};

std::ostream &operator<<(std::ostream &os, const ObjectHash &hash);

} // namespace ark

namespace std {
template <> struct hash<ark::ObjectHash> {
  size_t operator()(const ark::ObjectHash &h) const noexcept {
    return std::hash<std::string>{}(h.hex());
  }
};
} // namespace std

#endif
