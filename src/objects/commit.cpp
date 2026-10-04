#include <ark/objects/commit.h>
#include <ark/util/strings.h>
#include <ark/util/time.h>

#include <charconv>
#include <chrono>
#include <ctime>
#include <sstream>

namespace ark {

Signature Signature::now(const Identity &who) {
  Signature sig;
  sig.who = who;
  sig.unixTime =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  sig.utcOffset = formatLocalUtcOffset();
  return sig;
}

std::string Signature::serialize() const {
  std::ostringstream out;
  out << who.name << " <" << who.email << "> " << unixTime << " " << utcOffset;
  return out.str();
}

std::string Commit::serialize() const {
  std::ostringstream buffer;
  buffer << "tree " << tree.hex() << "\n";
  for (const ObjectHash &parent : parents) {
    buffer << "parent " << parent.hex() << "\n";
  }
  buffer << "committer " << committer.serialize() << "\n";
  buffer << message << "\n";
  return buffer.str();
}

namespace {

std::string_view takeLine(std::string_view body, std::size_t &pos) {
  if (pos >= body.size())
    return {};
  std::size_t end = body.find('\n', pos);
  std::string_view line;
  if (end == std::string_view::npos) {
    line = body.substr(pos);
    pos = body.size();
  } else {
    line = body.substr(pos, end - pos);
    pos = end + 1;
  }
  return line;
}

std::string secondToken(const std::string &line) {
  std::vector<std::string> values = text::splitKeepEmpty(line, ' ');
  return values.size() > 1 ? values[1] : "";
}

Signature parseSignature(std::string_view text) {
  Signature sig;
  std::size_t close = text.rfind('>');
  std::size_t open =
      close == std::string_view::npos ? close : text.rfind('<', close);
  if (open == std::string_view::npos) {
    sig.who.name = std::string(text);
    return sig;
  }
  std::string_view name = text.substr(0, open);
  if (!name.empty() && name.back() == ' ')
    name.remove_suffix(1);
  sig.who.name = std::string(name);
  sig.who.email = std::string(text.substr(open + 1, close - open - 1));

  std::string rest(text.substr(close + 1));
  std::vector<std::string> parts = text::splitKeepEmpty(text::trim(rest), ' ');
  if (!parts.empty()) {
    std::int64_t value = 0;
    const std::string &t = parts[0];
    std::from_chars(t.data(), t.data() + t.size(), value);
    sig.unixTime = value;
  }
  if (parts.size() > 1)
    sig.utcOffset = parts[1];
  return sig;
}

} // namespace

Commit Commit::parse(std::string_view body) {
  Commit commit;
  std::size_t pos = 0;
  std::string line(takeLine(body, pos));
  commit.tree = ObjectHash::unchecked(secondToken(line));

  line = std::string(takeLine(body, pos));
  while (text::startsWith(line, "parent")) {
    commit.parents.push_back(ObjectHash::unchecked(secondToken(line)));
    line = std::string(takeLine(body, pos));
  }

  if (text::startsWith(line, "committer ")) {
    commit.committer = parseSignature(std::string_view(line).substr(10));
    std::string_view rest = body.substr(pos);
    if (!rest.empty() && rest.back() == '\n')
      rest.remove_suffix(1);
    commit.message = std::string(rest);
  }
  return commit;
}

} // namespace ark
