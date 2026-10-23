#include <ark/core/ref_name.h>

namespace ark {

namespace {

constexpr std::string_view kHeadsPrefix = "refs/heads/";
constexpr std::string_view kRefsPrefix = "refs/";

bool isValidName(std::string_view name) {
  if (name.empty())
    return false;
  if (name.find("..") != std::string_view::npos)
    return false;
  for (char c : name) {
    unsigned char u = static_cast<unsigned char>(c);
    if (u <= 0x20 || u == 0x7f)
      return false;
    switch (c) {
    case '~':
    case '^':
    case ':':
    case '?':
    case '*':
    case '[':
    case '\\':
      return false;
    default:
      break;
    }
  }
  if (name.front() == '-' || name.front() == '/' || name.back() == '/')
    return false;
  constexpr std::string_view lock = ".lock";
  if (name.size() >= lock.size() &&
      name.compare(name.size() - lock.size(), lock.size(), lock) == 0)
    return false;
  return true;
}

} // namespace

std::optional<BranchName> BranchName::parse(std::string_view name) {
  if (!isValidName(name))
    return std::nullopt;
  return BranchName(std::string(name));
}

RefName BranchName::toRefName() const {
  return RefName::unchecked(std::string(kHeadsPrefix) + name_);
}

std::optional<RefName> RefName::parse(std::string_view name) {
  if (name.compare(0, kRefsPrefix.size(), kRefsPrefix) != 0)
    return std::nullopt;
  if (!isValidName(name))
    return std::nullopt;
  return RefName(std::string(name));
}

std::optional<BranchName> RefName::asBranch() const {
  if (name_.compare(0, kHeadsPrefix.size(), kHeadsPrefix) != 0)
    return std::nullopt;
  if (name_.size() == kHeadsPrefix.size())
    return std::nullopt;
  return BranchName::unchecked(name_.substr(kHeadsPrefix.size()));
}

std::ostream &operator<<(std::ostream &os, const BranchName &name) {
  return os << name.str();
}

std::ostream &operator<<(std::ostream &os, const RefName &name) {
  return os << name.str();
}

} // namespace ark
