#ifndef ARK_OBJECTS_COMMIT_H
#define ARK_OBJECTS_COMMIT_H

#include <ark/core/object_hash.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ark {

struct Identity {
  std::string name;
  std::string email;

  friend bool operator==(const Identity &a, const Identity &b) {
    return a.name == b.name && a.email == b.email;
  }
};

struct Signature {
  Identity who;
  std::int64_t unixTime = 0;
  std::string utcOffset;

  static Signature now(const Identity &who);
  // "<name> <<email>> <unix time> <+HHMM>"
  std::string serialize() const;

  friend bool operator==(const Signature &a, const Signature &b) {
    return a.who == b.who && a.unixTime == b.unixTime &&
           a.utcOffset == b.utcOffset;
  }
};

struct Commit {
  ObjectHash tree = ObjectHash::unchecked("");
  std::vector<ObjectHash> parents;
  Signature committer;
  std::string message;

  // The commit object body, without the object frame.
  std::string serialize() const;
  static Commit parse(std::string_view body);

  friend bool operator==(const Commit &a, const Commit &b) {
    return a.tree == b.tree && a.parents == b.parents &&
           a.committer == b.committer && a.message == b.message;
  }
};

} // namespace ark

#endif
