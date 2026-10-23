#ifndef ARK_CORE_REF_NAME_H
#define ARK_CORE_REF_NAME_H

#include <ark/core/object_hash.h>

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>

namespace ark {

class RefName;

// Name validation rules (shared by BranchName and RefName): non-empty; no "..";
// no spaces or control characters; none of ~ ^ : ? * [ \ ; no leading '-' or
// '/'; no trailing '/' or ".lock".
class BranchName {
public:
  // Skips validation; for existing callers that have not been migrated.
  static BranchName unchecked(std::string name) {
    return BranchName(std::move(name));
  }
  static std::optional<BranchName> parse(std::string_view name);

  const std::string &str() const { return name_; }
  RefName toRefName() const;

  friend bool operator==(const BranchName &a, const BranchName &b) {
    return a.name_ == b.name_;
  }
  friend bool operator!=(const BranchName &a, const BranchName &b) {
    return a.name_ != b.name_;
  }
  friend bool operator<(const BranchName &a, const BranchName &b) {
    return a.name_ < b.name_;
  }

private:
  explicit BranchName(std::string name) : name_(std::move(name)) {}
  std::string name_;
};

// A full reference name such as "refs/heads/main".
class RefName {
public:
  static RefName unchecked(std::string name) { return RefName(std::move(name)); }
  static std::optional<RefName> parse(std::string_view name);

  const std::string &str() const { return name_; }
  std::optional<BranchName> asBranch() const;

  friend bool operator==(const RefName &a, const RefName &b) {
    return a.name_ == b.name_;
  }
  friend bool operator!=(const RefName &a, const RefName &b) {
    return a.name_ != b.name_;
  }
  friend bool operator<(const RefName &a, const RefName &b) {
    return a.name_ < b.name_;
  }

private:
  explicit RefName(std::string name) : name_(std::move(name)) {}
  std::string name_;
};

// HEAD is either a symbolic ref or a detached commit hash.
using HeadTarget = std::variant<RefName, ObjectHash>;

std::ostream &operator<<(std::ostream &os, const BranchName &name);
std::ostream &operator<<(std::ostream &os, const RefName &name);

} // namespace ark

#endif
