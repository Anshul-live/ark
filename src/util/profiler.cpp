#include <ark/util/profiler.h>

#ifdef ARK_PROFILER

#include <cstdlib>
#include <fstream>
#include <map>

namespace ark::prof {

namespace {

struct Stat {
  std::uint64_t calls = 0;
  std::uint64_t inclusiveNs = 0;
  std::uint64_t exclusiveNs = 0;
};

// Ark is single-threaded, so one global stack of open scopes is enough.
ScopeTimer *current = nullptr;
std::map<std::string, Stat> scopes;
std::map<std::string, std::uint64_t> counters;

bool readEnabled() { return std::getenv("ARK_PROFILE") != nullptr; }

} // namespace

bool enabled() {
  static const bool on = readEnabled();
  return on;
}

ScopeTimer::ScopeTimer(const char *name)
    : name_(name), active_(enabled()), parent_(nullptr) {
  if (!active_)
    return;
  parent_ = current;
  current = this;
  start_ = std::chrono::steady_clock::now();
}

ScopeTimer::~ScopeTimer() {
  if (!active_)
    return;
  auto elapsed = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now() - start_)
          .count());
  Stat &stat = scopes[name_];
  stat.calls++;
  stat.inclusiveNs += elapsed;
  stat.exclusiveNs += elapsed - childNs_;
  current = parent_;
  if (parent_)
    parent_->childNs_ += elapsed;
}

void count(const char *name, std::uint64_t amount) {
  counters[name] += amount;
}

void dump(const std::string &command) {
  if (!enabled())
    return;
  const char *path = std::getenv("ARK_PROFILE");
  std::ofstream out(path, std::ios::app);
  for (const auto &[name, stat] : scopes) {
    out << command << '\t' << "scope" << '\t' << name << '\t' << stat.calls
        << '\t' << stat.inclusiveNs << '\t' << stat.exclusiveNs << '\n';
  }
  for (const auto &[name, value] : counters) {
    out << command << '\t' << "counter" << '\t' << name << '\t' << value
        << '\n';
  }
  scopes.clear();
  counters.clear();
}

} // namespace ark::prof

#endif // ARK_PROFILER
